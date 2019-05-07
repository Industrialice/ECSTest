#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
    constexpr bool IsMTECS = false;

    ui32 PhysicsSentKeys = 0;
    ui32 PhysicsReceivedRendererKeys = 0;
    ui32 PhysicsReceivedPhysicsKeys = 0;

    ui32 RendererSentKeys = 0;
    ui32 RendererReceivedRendererKeys = 0;
    ui32 RendererReceivedPhysicsKeys = 0;
}

COMPONENT(Position)
{
    Vector3 position;
};

INDIRECT_SYSTEM(PhysicsSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<Position> &);

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
                ASSUME(key->keyState == ControlAction::Key::KeyState::Pressed);
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

    virtual void Update(Environment &env) override
    {
        ControlAction::Key key;
        key.key = KeyCode::C;
        key.keyState = ControlAction::Key::KeyState::Pressed;

        env.keyController->Dispatch({key, {}, DeviceTypes::MouseKeyboard});

        ++PhysicsSentKeys;
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override {}
};

INDIRECT_SYSTEM(RendererSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(const Array<Position> &);

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
                ASSUME(key->keyState == ControlAction::Key::KeyState::Pressed);
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

    virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override {}
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

int main()
{
    StdLib::Initialization::Initialize({});

    auto logger = make_shared<Logger<string_view, true>>();
    auto handle0 = logger->OnMessage(LogRecipient);
    auto stream = make_unique<EntitiesStream>();

    auto idGenerator = EntityIDGenerator{};
    auto manager = SystemsManager::New(IsMTECS, logger);

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
        if (info.executedTimes > 100)
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

    system("pause");
}