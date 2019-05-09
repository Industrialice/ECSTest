#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
    constexpr bool IsMTECS = false;
    constexpr ui32 FramesToWait = 100;

    ui32 PhysicsSentKeys = 0;
    ui32 PhysicsReceivedRendererKeys = 0;
    ui32 PhysicsReceivedPhysicsKeys = 0;

    ui32 RendererSentKeys = 0;
    ui32 RendererReceivedRendererKeys = 0;
    ui32 RendererReceivedPhysicsKeys = 0;

    #define PHYSICS_INDIRECT
    #define RENDERER_INDIRECT

    #ifdef PHYSICS_INDIRECT
        #define PHYSICS_DECLARE INDIRECT_SYSTEM
        #define PHYSICS_ACCEPT INDIRECT_ACCEPT_COMPONENTS
    #else
        #define PHYSICS_DECLARE DIRECT_SYSTEM
        #define PHYSICS_ACCEPT DIRECT_ACCEPT_COMPONENTS
    #endif

    COMPONENT(Position)
    {
        Vector3 position;
    };

    PHYSICS_DECLARE(PhysicsSystem)
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
                //env.logger.Message(LogLevels::Info, "Received key %u\n", key->key);
            }
            else
            {
                SOFTBREAK;
            }

            return false;
        }

    #ifdef PHYSICS_INDIRECT
		using IndirectSystem::ProcessMessages;
        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override {}
        virtual void Update(Environment &env) override;
    #endif
    };

#ifdef PHYSICS_INDIRECT
    void PhysicsSystem::Update(Environment &env)
    #else
    void PhysicsSystem::Update(Environment &env, Array<Position> &)
    #endif
    {
        ControlAction::Key key;
        key.key = KeyCode::C;
        key.keyState = ControlAction::Key::KeyState::Released;

        env.keyController->Dispatch({key, {}, DeviceTypes::MouseKeyboard});

        ++PhysicsSentKeys;
    }

    INDIRECT_SYSTEM(RendererSystem)
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
                //env.logger.Message(LogLevels::Info, "Received key %u\n", key->key);
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

		using IndirectSystem::ProcessMessages;
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
}

void KeyControllerTests()
{
    StdLib::Initialization::Initialize({});

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