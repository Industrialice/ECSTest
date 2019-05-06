#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
    constexpr bool IsMTECS = false;
    constexpr ui32 EntitiesToTest = 32768;
    constexpr ui32 PhysicsUpdatesPerFrame = 100;
    constexpr ui32 RendererDrawPerFrame = 100;
}

COMPONENT(Physics)
{
    f32 mass;
    f32 linearDamping;
    f32 angularDamping;
    Vector3 centerOfMass;
    Vector3 inertiaTensor;
    Quaternion inertiaTensorRotation;
    Vector3 linearVelocity;
    Vector3 angularVelocity;
    ui32 solverIterations;
    ui32 solverVelocityIterations;
    f32 maxAngularVelocity;
    f32 sleepThreshold;
    f32 wakeCounter;
    f32 contactOffset;
    f32 restOffset;
};

COMPONENT(MeshRenderer)
{
    std::array<char, 260> mesh;
};

COMPONENT(Position)
{
    Vector3 position;

    Position() = default;
    Position(const Vector3 &position) : position(position) {}
};

COMPONENT(Rotation)
{
    Quaternion rotation;

    Rotation() = default;
    Rotation(const Quaternion &rotation) : rotation(rotation) {}
};

COMPONENT(MeshCollider)
{
    bool isTrigger;
    std::array<char, 260> mesh;
};

INDIRECT_SYSTEM(PhysicsSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<Position> &, Array<Rotation> &, const Array<MeshCollider> &, const Array<Physics> &);

    struct Data
    {
        Vector3 pos;
        Quaternion rot;
        Physics properties;
    };

    virtual void Update(Environment &env) override
    {
        if (_updateList.empty())
        {
            ui32 count = 0;
            for (const auto &[id, data] : _entities)
            {
                _updateList.push_back({id, std::cref(data)});

                if (++count == PhysicsUpdatesPerFrame)
                {
                    break;
                }
            }
        }

        for (const auto &[id, data] : _updateList)
        {
            env.messageBuilder.ComponentChanged(id, Position{data.pos});
            env.messageBuilder.ComponentChanged(id, Rotation{data.rot});
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
    {
        for (auto &entry : stream)
        {
            Data data;
            data.pos = entry.GetComponent<Position>().position;
            data.rot = entry.GetComponent<Rotation>().rotation;
            data.properties = entry.GetComponent<Physics>();
            _entities[entry.entityID] = data;
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
    {
        for (auto &entry : stream)
        {
            Data data;
            data.pos = entry.GetComponent<Position>().position;
            data.rot = entry.GetComponent<Rotation>().rotation;
            data.properties = entry.GetComponent<Physics>();
            _entities[entry.entityID] = data;
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
    {
        if (stream.Type() == Position::GetTypeId())
        {
            for (auto &entry : stream)
            {
                _entities[entry.entityID].pos = entry.component.Cast<Position>().position;
            }
        }
        else if (stream.Type() == Rotation::GetTypeId())
        {
            for (auto &entry : stream)
            {
                _entities[entry.entityID].rot = entry.component.Cast<Rotation>().rotation;
            }
        }
        else if (stream.Type() == Physics::GetTypeId())
        {
            for (auto &entry : stream)
            {
                _entities[entry.entityID].properties = entry.component.Cast<Physics>();
            }
        }
        else if (stream.Type() == MeshCollider::GetTypeId())
        {
        }
        else
        {
            HARDBREAK;
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
    {
        if (stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == Physics::GetTypeId() || stream.Type() == MeshCollider::GetTypeId())
        {
            for (auto &entry : stream)
            {
                _entities.erase(entry.entityID);
            }
            _updateList.clear();
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
    {
        for (auto &entry : stream)
        {
            _entities.erase(entry);
        }
        _updateList.clear();
    }

private:
    std::unordered_map<EntityID, Data> _entities{};
    std::vector<std::pair<EntityID, std::remove_reference_t<Data>>> _updateList{};
};

INDIRECT_SYSTEM(RendererSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(const Array<Position> &, const Array<Rotation> &, const Array<MeshRenderer> &);

    struct Data
    {
        Vector3 pos;
        Quaternion rot;
        MeshRenderer mesh;
    };

    virtual void Update(Environment &env) override
    {
        ui32 count = 0;
        for (const auto &[id, data] : _entities)
        {
            ++count;
            if (count == RendererDrawPerFrame)
            {
                break;
            }
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
    {
        for (auto &entry : stream)
        {
            Data data;
            data.pos = entry.GetComponent<Position>().position;
            data.rot = entry.GetComponent<Rotation>().rotation;
            data.mesh = entry.GetComponent<MeshRenderer>();
            _entities[entry.entityID] = data;
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
    {
        for (auto &entry : stream)
        {
            Data data;
            data.pos = entry.GetComponent<Position>().position;
            data.rot = entry.GetComponent<Rotation>().rotation;
            data.mesh = entry.GetComponent<MeshRenderer>();
            _entities[entry.entityID] = data;
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
    {
        if (stream.Type() == Position::GetTypeId())
        {
            for (auto &entry : stream)
            {
                _entities[entry.entityID].pos = entry.component.Cast<Position>().position;
            }
        }
        else if (stream.Type() == Rotation::GetTypeId())
        {
            for (auto &entry : stream)
            {
                _entities[entry.entityID].rot = entry.component.Cast<Rotation>().rotation;
            }
        }
        else if (stream.Type() == MeshRenderer::GetTypeId())
        {
            for (auto &entry : stream)
            {
                _entities[entry.entityID].mesh = entry.component.Cast<MeshRenderer>();
            }
        }
        else
        {
            HARDBREAK;
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
    {
        if (stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == MeshRenderer::GetTypeId())
        {
            for (auto &entry : stream)
            {
                _entities.erase(entry.entityID);
            }
        }
    }

    virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
    {
        for (auto &entry : stream)
        {
            _entities.erase(entry);
        }
    }

private:
    std::unordered_map<EntityID, Data> _entities{};
};

static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
{
    for (uiw index = 0; index < EntitiesToTest; ++index)
    {
        EntitiesStream::EntityData entity;

        Position pos;
        pos.position = {(f32)rand(), (f32)rand(), (f32)rand()};
        entity.AddComponent(pos);

        Rotation rot;
        rot.rotation = Quaternion::FromEuler({(f32)rand(), (f32)rand(), (f32)rand()});
        entity.AddComponent(rot);

        Physics phy;
        entity.AddComponent(phy);

        MeshRenderer meshRenderer;
        entity.AddComponent(meshRenderer);

        MeshCollider collider;
        entity.AddComponent(collider);

        stream.AddEntity(entityIdGenerator.Generate(), move(entity));
    }
}

#define PRINT_BACK "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"

int main()
{
    StdLib::Initialization::Initialize({});

    auto logger = make_shared<Logger<string_view, true>>();
    auto handle0 = logger->OnMessage(LogRecipient);

    auto idGenerator = EntityIDGenerator{};
    auto manager = SystemsManager::New(IsMTECS, logger);
    auto stream = make_unique<EntitiesStream>();

    auto before = TimeMoment::Now();
    GenerateScene(idGenerator, *manager, *stream);
    auto after = TimeMoment::Now();
    printf("Generating scene took %.2lfs\n", (after - before).ToSec());

    auto physicsPipeline = manager->CreatePipeline(16.666_ms, false);
    auto rendererPipeline = manager->CreatePipeline(nullopt, false);

    manager->Register<PhysicsSystem>(physicsPipeline);
    manager->Register<RendererSystem>(rendererPipeline);

    vector<WorkerThread> workers;
    if (IsMTECS)
    {
        workers.resize(SystemInfo::LogicalCPUCores());
    }

    manager->Start(move(idGenerator), move(workers), move(stream));

    TimeDifference lastDifference = 0_s;
    ui32 lastExecutedRenderer = 0, lastExecutedPhysics = 0;

    std::vector<f32> fpsHistory{};

    for (;;)
    {
        auto managerInfo = manager->GetManagerInfo();
        auto timeDiff = managerInfo.timeSinceStart - lastDifference;
        if (timeDiff >= 1_s)
        {
            auto rendererInfo = manager->GetPipelineInfo(rendererPipeline);
            ui32 rexecDiff = rendererInfo.executedTimes - lastExecutedRenderer;
            
            auto physicsInfo = manager->GetPipelineInfo(physicsPipeline);
            ui32 pexecDiff = physicsInfo.executedTimes - lastExecutedPhysics;

            f32 rfps = (f32)rexecDiff / timeDiff.ToSec();
            f32 pfps = (f32)pexecDiff / timeDiff.ToSec();
            printf(PRINT_BACK "renderer %.2f fps, physics %.2f fps   ", rfps, pfps);
            
            fpsHistory.push_back(rfps);
            lastExecutedRenderer = rendererInfo.executedTimes;
            lastExecutedPhysics = physicsInfo.executedTimes;
            lastDifference = managerInfo.timeSinceStart;
        }
        std::this_thread::yield();
    }

    manager->Stop(true);

    f32 avg = std::accumulate(fpsHistory.begin(), fpsHistory.end(), 0);
    printf("\nAverage FPS: %.2lf\n", avg / fpsHistory.size());

    system("pause");
}