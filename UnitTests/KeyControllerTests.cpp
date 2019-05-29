#include "PreHeader.hpp"

using namespace ECSTest;

class KeyControllerTestsClass
{
    static constexpr bool IsMTECS = false;
    static constexpr ui32 FramesToWait = 100;

    static inline ui32 PhysicsSentKeys;
	static inline ui32 PhysicsReceivedRendererKeys;
	static inline ui32 PhysicsReceivedPhysicsKeys;

	static inline ui32 RendererSentKeys;
	static inline ui32 RendererReceivedRendererKeys;
	static inline ui32 RendererReceivedPhysicsKeys;

public:
	KeyControllerTestsClass()
	{
		PhysicsSentKeys = 0;
		PhysicsReceivedRendererKeys = 0;
		PhysicsReceivedPhysicsKeys = 0;

		RendererSentKeys = 0;
		RendererReceivedRendererKeys = 0;
		RendererReceivedPhysicsKeys = 0;

		auto logger = make_shared<Logger<string_view, true>>();
		auto handle0 = logger->OnMessage(LogRecipient);
		auto stream = make_unique<EntitiesStream>();

		auto idGenerator = EntityIDGenerator{};
		auto manager = SystemsManager::New(IsMTECS, logger);

		GenerateScene(idGenerator, *manager, *stream);

		auto pipeline = manager->CreatePipeline(nullopt, false);

		auto physicsSystem = make_unique<PhysicsSystem>();
		physicsSystem->SetKeyController(KeyController::New());

		auto rendererSystem = make_unique<RendererSystem>();
		rendererSystem->SetKeyController(KeyController::New());

		manager->Register(move(physicsSystem), pipeline);
		manager->Register(move(rendererSystem), pipeline);

		vector<WorkerThread> workers;
		if (IsMTECS)
		{
			workers.resize(SystemInfo::LogicalCPUCores());
		}

		manager->Start(move(idGenerator), move(workers), move(stream));

		for (;;)
		{
			auto info = manager->GetPipelineInfo(pipeline);
			if (info.executedTimes > FramesToWait)
			{
				break;
			}
			std::this_thread::yield();
		}

		manager->Stop(true);

		ASSUME(PhysicsReceivedRendererKeys >= RendererSentKeys - 1);
		ASSUME(PhysicsReceivedPhysicsKeys == PhysicsSentKeys);
		ASSUME(RendererReceivedRendererKeys == RendererSentKeys);
		ASSUME(RendererReceivedPhysicsKeys >= PhysicsSentKeys - 1);
	}

    #define PHYSICS_INDIRECT
    #define RENDERER_INDIRECT

    #ifdef PHYSICS_INDIRECT
        #define PHYSICS_DECLARE IndirectSystem
    #else
        #define PHYSICS_DECLARE DirectSystem
    #endif

    struct Position : Component<Position>
    {
        Vector3 position;
    };

    struct PhysicsSystem : PHYSICS_DECLARE<PhysicsSystem>
    {
        void Accept(Array<Position> &);

        virtual bool ControlInput(Environment &env, const ControlAction &input) override
        {
            if (auto key = input.Get<ControlAction::Key>(); key)
            {
                if (key->key == KeyCode::B)
                {
                    ASSUME(key->keyState == ControlAction::Key::KeyState::Repeated);
                    ++PhysicsReceivedRendererKeys;
                }
                else if (key->key == KeyCode::C)
                {
                    ASSUME(key->keyState == ControlAction::Key::KeyState::Released);
                    ++PhysicsReceivedPhysicsKeys;
                }
                else
                {
                    SOFTBREAK;
                }
                //env.logger.Info("Received key %u\n", key->key);
            }
            else
            {
                SOFTBREAK;
            }

            return false;
        }

    #ifdef PHYSICS_INDIRECT
		using BaseIndirectSystem::ProcessMessages;
        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override {}
        virtual void Update(Environment &env) override;
    #endif
    };

    struct RendererSystem : IndirectSystem<RendererSystem>
    {
		void Accept(const Array<Position> &);

        virtual bool ControlInput(Environment &env, const ControlAction &input) override
        {
            if (auto key = input.Get<ControlAction::Key>(); key)
            {
                if (key->key == KeyCode::B)
                {
                    ASSUME(key->keyState == ControlAction::Key::KeyState::Repeated);
                    ++RendererReceivedRendererKeys;
                }
                else if (key->key == KeyCode::C)
                {
                    ASSUME(key->keyState == ControlAction::Key::KeyState::Released);
                    ++RendererReceivedPhysicsKeys;
                }
                else
                {
                    SOFTBREAK;
                }
                //env.logger.Info("Received key %u\n", key->key);
            }
            else
            {
                SOFTBREAK;
            }

            return false;
        }

        virtual void Update(Environment &env) override
        {
            ControlAction::Key key;
            key.key = KeyCode::B;
            key.keyState = ControlAction::Key::KeyState::Repeated;

            env.keyController->Dispatch({key, {}, DeviceTypes::MouseKeyboard});

            ++RendererSentKeys;
        }

		using BaseIndirectSystem::ProcessMessages;
        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override {}
        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override {}
    };

    static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
    {
        for (uiw index = 0; index < 10; ++index)
        {
            EntitiesStream::EntityData entity;

            Position pos;
            pos.position = {(f32)rand(), (f32)rand(), (f32)rand()};
            entity.AddComponent(pos);

            stream.AddEntity(entityIdGenerator.Generate(), move(entity));
        }
    }
};

#ifdef PHYSICS_INDIRECT
	void KeyControllerTestsClass::PhysicsSystem::Update(Environment &env)
#else
	void KeyControllerTestsClass::PhysicsSystem::Update(Environment &env, Array<Position> &)
#endif
{
	ControlAction::Key key;
	key.key = KeyCode::C;
	key.keyState = ControlAction::Key::KeyState::Released;

	env.keyController->Dispatch({key, {}, DeviceTypes::MouseKeyboard});

	++PhysicsSentKeys;
}

void KeyControllerTests()
{
    StdLib::Initialization::Initialize({});
	KeyControllerTestsClass test;
}