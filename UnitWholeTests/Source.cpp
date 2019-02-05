#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>
#include <set>

using namespace ECSTest;

namespace MonitoringStats
{
    ui32 receivedEntityAddedCount = 0;
    ui32 receivedComponentChangedCount = 0;
    ui32 receivedEntityRemovedCount = 0;
    ui32 receivedTest0ChangedCount = 0;
    ui32 receivedTest1ChangedCount = 0;
    ui32 receivedTest2ChangedCount = 0;

    ui32 testIndirectSystem0UpdatedTimes = 0;
    ui32 testIndirectSystem1UpdatedTimes = 0;
    ui32 monitoringUpdatedTimes = 0;
}

COMPONENT(TestComponent0)
{
    ui32 value;
};

COMPONENT(TestComponent1)
{
    ui32 value;
};

NONUNIQUE_COMPONENT(TestComponent2)
{
    ui32 value;
};

INDIRECT_SYSTEM(TestIndirectSystem0)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<TestComponent0> &);

private:
    std::set<EntityID> _entities{};
};

void TestIndirectSystem0::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entry : stream)
    {
        _entities.insert(entry.entityID);
    }
}

void TestIndirectSystem0::ProcessMessages(const MessageStreamComponentChanged &stream)
{
}

void TestIndirectSystem0::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entry : stream)
    {
        _entities.erase(entry);
    }
}

void TestIndirectSystem0::Update(Environment &env, MessageBuilder &messageBuilder)
{
    for (auto &entry : _entities)
    {
        TestComponent0 c;
        c.value = 1;
        messageBuilder.ComponentChanged(entry, c, 0);
    }

    _entities.clear();

    ++MonitoringStats::testIndirectSystem0UpdatedTimes;
}

INDIRECT_SYSTEM(TestIndirectSystem1)
{
    INDIRECT_ACCEPT_COMPONENTS(const Array<TestComponent1> &);
};

void TestIndirectSystem1::ProcessMessages(const MessageStreamEntityAdded &stream)
{}

void TestIndirectSystem1::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void TestIndirectSystem1::ProcessMessages(const MessageStreamEntityRemoved &stream)
{}

void TestIndirectSystem1::Update(Environment &env, MessageBuilder &messageBuilder)
{
    ++MonitoringStats::testIndirectSystem1UpdatedTimes;
}

INDIRECT_SYSTEM(MonitoringSystem)
{
    INDIRECT_ACCEPT_COMPONENTS();

private:
};

void MonitoringSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entry : stream)
    {
        ++MonitoringStats::receivedEntityAddedCount;

        for (auto &component : entry.components)
        {
            if (component.type == TestComponent0::GetTypeId())
            {
                auto casted = (TestComponent0 *)component.data;
                ASSUME(casted->value == 0);
            }
        }
    }
}

void MonitoringSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
    for (auto &entry : stream)
    {
        ++MonitoringStats::receivedComponentChangedCount;

        if (entry.component.type == TestComponent0::GetTypeId())
        {
            ++MonitoringStats::receivedTest0ChangedCount;

            auto casted = (TestComponent0 *)entry.component.data;
            ASSUME(casted->value == 1);
        }
        else if (entry.component.type == TestComponent1::GetTypeId())
        {
            ++MonitoringStats::receivedTest1ChangedCount;
        }
        else if (entry.component.type == TestComponent2::GetTypeId())
        {
            ++MonitoringStats::receivedTest2ChangedCount;
        }
    }
}

void MonitoringSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entry : stream)
    {
        ++MonitoringStats::receivedEntityRemovedCount;
    }
}

void MonitoringSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{
    ++MonitoringStats::monitoringUpdatedTimes;
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
    for (uiw index = 0; index < 100; ++index)
    {
        TestEntities::PreStreamedEntity entity;

        if (index < 75)
        {
            TestComponent0 c;
            c.value = 0;
            StreamComponent(c, entity);
        }

        if (index < 50)
        {
            TestComponent1 c;
            c.value = 1;
            StreamComponent(c, entity);
        }

        if (index < 25)
        {
            TestComponent2 c;
            c.value = 2;
            StreamComponent(c, entity);
        }

        if (index < 10)
        {
            TestComponent2 c;
            c.value = 3;
            StreamComponent(c, entity);
        }

        stream.AddEntity(idGenerator.Generate(), move(entity));
    }
}

static void PrintStreamInfo(EntitiesStream &stream, bool isFirstPass)
{
    ui32 entitiesCount = 0;
    ui32 test0Count = 0;
    ui32 test1Count = 0;
    ui32 test2Count = 0;
    for (auto streamed = stream.Next(); streamed; streamed = stream.Next())
    {
        ++entitiesCount;

        for (const auto &component : streamed->components)
        {
            if (component.type == TestComponent0::GetTypeId())
            {
                ++test0Count;
            }
            else if (component.type == TestComponent1::GetTypeId())
            {
                ++test1Count;
            }
            else if (component.type == TestComponent2::GetTypeId())
            {
                ++test2Count;
            }
        }
    }

    ASSUME(entitiesCount == 100);
    ASSUME(test0Count == 75);
    ASSUME(test1Count == 50);
    ASSUME(test2Count == 25 + 10);

    ASSUME(MonitoringStats::receivedEntityAddedCount == 100);
    ASSUME(MonitoringStats::receivedEntityRemovedCount == 0);
    ASSUME(MonitoringStats::receivedComponentChangedCount == 75);
    ASSUME(MonitoringStats::receivedTest0ChangedCount == 75);
    ASSUME(MonitoringStats::receivedTest1ChangedCount == 0);
    ASSUME(MonitoringStats::receivedTest2ChangedCount == 0);

    printf("finished checking stream, pass %s\n", (isFirstPass ? "first" : "second"));

    printf("ECS info:\n");
    printf("  entities: %u\n", entitiesCount);
    printf("  test0 components: %u\n", test0Count);
    printf("  test1 components: %u\n", test1Count);
    printf("  test2 components: %u\n", test2Count);
    printf("  test 0 system updated times: %u\n", MonitoringStats::testIndirectSystem0UpdatedTimes);
    printf("  test 1 system updated times: %u\n", MonitoringStats::testIndirectSystem0UpdatedTimes);
    printf("  monitoring system updated times: %u\n", MonitoringStats::monitoringUpdatedTimes);
    printf("\n");
}

int main()
{
    StdLib::Initialization::Initialize({});

    auto stream = make_unique<TestEntities>();
    auto manager = SystemsManager::New();
    EntityIDGenerator idGenerator;

    GenerateScene(idGenerator, *manager, *stream);

    auto testPipelineGroup0 = manager->CreatePipelineGroup(1000'0000, false);
    auto testPipelineGroup1 = manager->CreatePipelineGroup(1000'0000, false);

    manager->Register(make_unique<TestIndirectSystem0>(), testPipelineGroup0);

    manager->Register(make_unique<TestIndirectSystem1>(), testPipelineGroup0);

    manager->Register(make_unique<MonitoringSystem>(), testPipelineGroup1);

    vector<WorkerThread> workers(SystemInfo::LogicalCPUCores());

    vector<unique_ptr<EntitiesStream>> streams;
    streams.push_back(move(stream));
    manager->Start(move(idGenerator), move(workers), move(streams));
    
    std::this_thread::sleep_for(500ms);

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