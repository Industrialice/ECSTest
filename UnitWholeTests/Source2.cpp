#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>
#include <set>

using namespace ECSTest;

namespace
{
    constexpr bool IsMultiThreadedECS = false;
}

COMPONENT(GeneratedComponent)
{
    ui32 value;
};

COMPONENT(ConsumerInfoComponent)
{
    ui32 received = 0;
};

INDIRECT_SYSTEM(GeneratorSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(SubtractiveComponent<GeneratedComponent>)
    {
        if (_leftToGenerate)
        {
            GeneratedComponent c;
            c.value = 15;
            env.messageBuilder.EntityAdded(env.idGenerator.Generate()).AddComponent(c);
            --_leftToGenerate;
        }
    }

private:
    ui32 _leftToGenerate = 50;
};

void GeneratorSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    SOFTBREAK;
}

void GeneratorSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{
    SOFTBREAK;
}

void GeneratorSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
    SOFTBREAK;
}

void GeneratorSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
{
    SOFTBREAK;
}

void GeneratorSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    SOFTBREAK;
}

INDIRECT_SYSTEM(ConsumerSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<GeneratedComponent> &)
    {
        if (_infoID.IsValid() == false)
        {
            _infoID = env.idGenerator.Generate();
            env.messageBuilder.EntityAdded(_infoID).AddComponent(_info);
        }
    }

private:
    EntityID _infoID{};
    ConsumerInfoComponent _info{};
};

void ConsumerSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entity : stream)
    {
        if (auto c = entity.FindComponent<GeneratedComponent>(); c)
        {
            ++_info.received;
            ASSUME(c->value == 15);
        }
    }
}

void ConsumerSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{
    SOFTBREAK;
}

void ConsumerSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
    SOFTBREAK;
}

void ConsumerSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
{
    SOFTBREAK;
}

void ConsumerSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    SOFTBREAK;
}

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
    for (uiw index = 0; index < EntitiesToTest; ++index)
    {
        TestEntities::PreStreamedEntity entity;

        if (IsPreGenerateTransform)
        {
            Transform t;
            t.position.x = rand() / (f32)RAND_MAX * 100 - 50;
            t.position.y = rand() / (f32)RAND_MAX * 100 - 50;
            t.position.z = rand() / (f32)RAND_MAX * 100 - 50;
            StreamComponent(t, entity);
        }

        string name = "Entity"s + std::to_string(index);
        Name n;
        strcpy_s(n.name.data(), n.name.size(), name.c_str());
        StreamComponent(n, entity);

        stream.AddEntity(idGenerator.Generate(), move(entity));
    }
}

static void PrintStreamInfo(EntitiesStream &stream, bool isFirstPass)
{
    if (!isFirstPass)
    {
        printf("\n-------------\n\n");
    }

    std::map<StableTypeId, ui32> componentCounts{};

    while (auto entity = stream.Next())
    {
        for (auto &c : entity->components)
        {
            componentCounts[c.type]++;

            if (c.type == AverageHeight::GetTypeId())
            {
                auto a = *(AverageHeight *)c.data;
                printf("average height %f based on %u sources\n", a.height, a.sources);
            }
            else if (c.type == HeightFixerInfo::GetTypeId())
            {
                auto i = *(HeightFixerInfo *)c.data;
                printf("height fixer run %u times, fixed %u heights\n", i.runTimes, i.heightsFixed);
            }
        }
    }

    printf("\n");
    for (auto &[id, count] : componentCounts)
    {
        if (id == Transform::GetTypeId())
        {
            ASSUME(count == EntitiesToTest);
        }
        else if (id == Name::GetTypeId())
        {
            ASSUME(count == EntitiesToTest);
        }
        else if (id == AverageHeight::GetTypeId())
        {
            ASSUME(count == 1);
        }
        else if (id == HeightFixerInfo::GetTypeId())
        {
            ASSUME(count == 1);
        }

        printf("%s count %u\n", id.Name(), count);
    }
}

int main()
{
    StdLib::Initialization::Initialize({});

    auto stream = make_unique<TestEntities>();
    auto manager = SystemsManager::New(IsMultiThreadedECS);
    EntityIDGenerator idGenerator;

    GenerateScene(idGenerator, *manager, *stream);

    auto testPipelineGroup0 = manager->CreatePipelineGroup(5_ms, false);
    auto testPipelineGroup1 = manager->CreatePipelineGroup(6.5_ms, false);

    manager->Register(make_unique<GeneratorSystem>(), testPipelineGroup0);
    manager->Register(make_unique<TransformHeightFixerSystem>(), testPipelineGroup0);
    if (IsUseDirectForFalling)
    {
        manager->Register(make_unique<TransformFallingDirectSystem>(), testPipelineGroup0);
    }
    else
    {
        manager->Register(make_unique<TransformFallingIndirectSystem>(), testPipelineGroup0);
    }
    manager->Register(make_unique<AverageHeightAnalyzerSystem>(), testPipelineGroup1);
    manager->Register(make_unique<CooldownUpdater>(), testPipelineGroup1);

    vector<WorkerThread> workers;
    if (IsMultiThreadedECS)
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

    std::this_thread::sleep_for(1500ms);

    manager->Pause(true);

    ecsstream = manager->StreamOut();

    PrintStreamInfo(*ecsstream, false);

    manager->Stop(true);

    system("pause");
}