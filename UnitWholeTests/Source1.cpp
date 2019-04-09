#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>
#include <set>

using namespace ECSTest;

COMPONENT(Name)
{
    array<char, 32> name;
};

COMPONENT(Transform)
{
    Vector3 position;
};

INDIRECT_SYSTEM(TransformGeneratorSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<Name> &, SubtractiveComponent<Transform>);

private:
    vector<EntityID> _entitiesToGenerate{};
};

void TransformGeneratorSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entity : stream)
    {
        _entitiesToGenerate.push_back(entity.entityID);
    }
}

void TransformGeneratorSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void TransformGeneratorSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entity : stream)
    {
        auto it = std::find(_entitiesToGenerate.begin(), _entitiesToGenerate.end(), entity);
        ASSUME(it != _entitiesToGenerate.end());
        _entitiesToGenerate.erase(it);
    }
}

void TransformGeneratorSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{
    for (auto &id : _entitiesToGenerate)
    {
        Transform t;
        t.position.x = rand() / (f32)RAND_MAX * 100 - 50;
        t.position.y = rand() / (f32)RAND_MAX * 100 - 50;
        t.position.z = rand() / (f32)RAND_MAX * 100 - 50;
    }
    _entitiesToGenerate.clear();
}

INDIRECT_SYSTEM(TransformHeightFixerSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<Transform> &);

private:
};

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{}

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{}

void TransformHeightFixerSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{}

INDIRECT_SYSTEM(TransformFallingSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<Transform> &);

private:
};

void TransformFallingSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{}

void TransformFallingSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void TransformFallingSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{}

void TransformFallingSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{}

INDIRECT_SYSTEM(AverageHeightAnalyzerSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(const Array<Transform> &);

private:
};

void AverageHeightAnalyzerSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{}

void AverageHeightAnalyzerSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void AverageHeightAnalyzerSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{}

void AverageHeightAnalyzerSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{}

using namespace ECSTest;

class TestEntities : public EntitiesStream
{
public:
    struct PreStreamedEntity
    {
        StreamedEntity streamed;
        vector<ComponentDesc> descs;
        vector<unique_ptr<ui8[]>> componentsData;

        PreStreamedEntity() = default;
        PreStreamedEntity(PreStreamedEntity &&) = default;
        PreStreamedEntity &operator = (PreStreamedEntity &&) = default;
    };

private:
    vector<PreStreamedEntity> _entities{};
    uiw _currentEntity{};

public:
    [[nodiscard]] virtual optional<StreamedEntity> Next() override
    {
        if (_currentEntity < _entities.size())
        {
            uiw index = _currentEntity++;
            return _entities[index].streamed;
        }
        return {};
    }

    void AddEntity(EntityID id, PreStreamedEntity &&entity)
    {
        _entities.emplace_back(move(entity));
        _entities.back().streamed.components = ToArray(_entities.back().descs);
        _entities.back().streamed.entityId = id;
    }
};

template <typename T> void StreamComponent(const T &component, TestEntities::PreStreamedEntity &preStreamed)
{
    EntitiesStream::ComponentDesc desc;
    auto componentData = make_unique<ui8[]>(sizeof(T));
    memcpy(componentData.get(), &component, sizeof(T));
    desc.alignmentOf = alignof(T);
    desc.isUnique = T::IsUnique();
    desc.sizeOf = sizeof(T);
    desc.type = T::GetTypeId();
    desc.data = componentData.get();
    preStreamed.componentsData.emplace_back(move(componentData));
    preStreamed.descs.emplace_back(desc);
}

static void GenerateScene(EntityIDGenerator &idGenerator, SystemsManager &manager, TestEntities &stream)
{
    for (uiw index = 0; index < 100; ++index)
    {
        TestEntities::PreStreamedEntity entity;


        stream.AddEntity(idGenerator.Generate(), move(entity));
    }
}

static void PrintStreamInfo(EntitiesStream &stream, bool isFirstPass)
{
}

int main()
{
    StdLib::Initialization::Initialize({});

    constexpr bool isMT = false;

    auto stream = make_unique<TestEntities>();
    auto manager = SystemsManager::New(isMT);
    EntityIDGenerator idGenerator;

    GenerateScene(idGenerator, *manager, *stream);

    auto testPipelineGroup0 = manager->CreatePipelineGroup(1_ms, false);
    auto testPipelineGroup1 = manager->CreatePipelineGroup(1.5_ms, false);

    manager->Register(make_unique<TransformGeneratorSystem>(), testPipelineGroup0);

    manager->Register(make_unique<TestIndirectSystem1>(), testPipelineGroup0);

    manager->Register(make_unique<TestIndirectSystem2>(), testPipelineGroup0);

    manager->Register(make_unique<MonitoringSystem>(), testPipelineGroup1);

    vector<WorkerThread> workers;
    if (isMT)
    {
        workers.resize(SystemInfo::LogicalCPUCores());
    }

    vector<unique_ptr<EntitiesStream>> streams;
    streams.push_back(move(stream));
    manager->Start(move(idGenerator), move(workers), move(streams));

    std::this_thread::sleep_for(2000ms);

    manager->Pause(true);

    auto ecsstream = manager->StreamOut();

    PrintStreamInfo(*ecsstream, true);

    manager->Resume();

    std::this_thread::sleep_for(500ms);

    manager->Pause(true);

    ecsstream = manager->StreamOut();

    PrintStreamInfo(*ecsstream, false);

    manager->Stop(true);

    system("pause");
}