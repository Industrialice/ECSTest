#include "PreHeader.hpp"
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

// cannot run in parallel with TestIndirectSystem2
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

void TestIndirectSystem0::ProcessMessages(const MessageStreamComponentAdded &stream)
{}

void TestIndirectSystem0::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void TestIndirectSystem0::ProcessMessages(const MessageStreamComponentRemoved &stream)
{}

void TestIndirectSystem0::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entry : stream)
    {
        _entities.erase(entry);
    }
}

void TestIndirectSystem0::Update(Environment &env)
{
    if (_entities.empty())
    {
        return;
    }

    TestComponent0 c;
    c.value = 1;
    env.messageBuilder.ComponentChanged(*_entities.begin(), c);
    _entities.erase(_entities.begin());
}

INDIRECT_SYSTEM(TestIndirectSystem1)
{
    INDIRECT_ACCEPT_COMPONENTS(const Array<TestComponent0> &, const Array<TestComponent1> &, const NonUnique<TestComponent2> *);

private:
    vector<pair<Archetype, EntityID>> _entities{};
    bool _isFirstUpdate = true;
};

void TestIndirectSystem1::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entry : stream)
    {
        _entities.push_back({stream.Archetype(), entry.entityID});
    }
}

void TestIndirectSystem1::ProcessMessages(const MessageStreamComponentAdded &stream)
{}

void TestIndirectSystem1::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void TestIndirectSystem1::ProcessMessages(const MessageStreamComponentRemoved &stream)
{}

void TestIndirectSystem1::ProcessMessages(const MessageStreamEntityRemoved &stream)
{}

void TestIndirectSystem1::Update(Environment &env)
{
    if (_isFirstUpdate)
    {
        ASSUME(_entities.size() == 50);
        _isFirstUpdate = false;
    }

    if (_entities.size() > 100)
    {
        env.messageBuilder.RemoveEntity(_entities.back().second, _entities.back().first);
        _entities.pop_back();
    }
}

// cannot run in parallel with TestIndirectSystem0
INDIRECT_SYSTEM(TestIndirectSystem2)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<TestComponent0> &);

private:
    ui32 _entitiesToAdd = 100;
};

void TestIndirectSystem2::ProcessMessages(const MessageStreamEntityAdded &stream)
{}

void TestIndirectSystem2::ProcessMessages(const MessageStreamComponentAdded &stream)
{}

void TestIndirectSystem2::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void TestIndirectSystem2::ProcessMessages(const MessageStreamComponentRemoved &stream)
{}

void TestIndirectSystem2::ProcessMessages(const MessageStreamEntityRemoved &stream)
{}

void TestIndirectSystem2::Update(Environment &env)
{
    if (_entitiesToAdd)
    {
        auto &componentBuilder = env.messageBuilder.AddEntity(env.entityIdGenerator.Generate());
        TestComponent0 c0;
        c0.value = 10;
        TestComponent1 c1;
        c1.value = 20;
        componentBuilder.AddComponent(c0).AddComponent(c1);
        --_entitiesToAdd;
    }
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
                ASSUME(casted->value == 0 || casted->value == 10);
            }
        }
    }
}

void MonitoringSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{}

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

void MonitoringSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
{}

void MonitoringSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entry : stream)
    {
        ++MonitoringStats::receivedEntityRemovedCount;
    }
}

void MonitoringSystem::Update(Environment &env)
{
}

using namespace ECSTest;

static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, TestEntities &stream)
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

        stream.AddEntity(entityIdGenerator.Generate(), move(entity));
    }
}

static void PrintStreamInfo(EntitiesStream &stream, bool isFirstPass)
{
    ui32 entitiesCount = 0;
    ui32 test0Count = 0;
    ui32 test1Count = 0;
    ui32 test2Count = 0;

    while (auto streamed = stream.Next())
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

    ASSUME(entitiesCount == 150);
    ASSUME(test0Count == 125);
    ASSUME(test1Count == 100);
    ASSUME(test2Count == 25 + 10);

    ASSUME(MonitoringStats::receivedEntityAddedCount == 200);
    ASSUME(MonitoringStats::receivedEntityRemovedCount == 50);
    ASSUME(MonitoringStats::receivedComponentChangedCount == 125);
    ASSUME(MonitoringStats::receivedTest0ChangedCount == 125);
    ASSUME(MonitoringStats::receivedTest1ChangedCount == 0);
    ASSUME(MonitoringStats::receivedTest2ChangedCount == 0);

    printf("finished checking stream, pass %s\n", (isFirstPass ? "first" : "second"));

    printf("ECS info:\n");
    printf("  entities: %u\n", entitiesCount);
    printf("  test0 components: %u\n", test0Count);
    printf("  test1 components: %u\n", test1Count);
    printf("  test2 components: %u\n", test2Count);
    printf("\n");
}

int main()
{
    StdLib::Initialization::Initialize({});

    constexpr bool isMT = false;

    auto stream = make_unique<TestEntities>();
    auto manager = SystemsManager::New(isMT);
    EntityIDGenerator entityIdGenerator;

    GenerateScene(entityIdGenerator, *manager, *stream);

    auto testPipeline0 = manager->CreatePipeline(1_ms, false);
    auto testPipeline1 = manager->CreatePipeline(1.5_ms, false);

    manager->Register<TestIndirectSystem0>(testPipeline0);

    manager->Register<TestIndirectSystem1>(testPipeline0);

    manager->Register<TestIndirectSystem2>(testPipeline0);

    manager->Register<MonitoringSystem>(testPipeline1);

    vector<WorkerThread> workers;
    if (isMT)
    {
        workers.resize(SystemInfo::LogicalCPUCores());
    }

    manager->Start(move(entityIdGenerator), move(workers), move(stream));
    
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