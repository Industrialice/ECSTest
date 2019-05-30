#include "PreHeader.hpp"

using namespace ECSTest;

class SyncTestsClass
{
	struct MonitoringStats
	{
		static inline ui32 receivedEntityAddedCount;
		static inline ui32 receivedComponentChangedCount;
		static inline ui32 receivedEntityRemovedCount;
		static inline ui32 receivedTest0ChangedCount;
		static inline ui32 receivedTest1ChangedCount;
		static inline ui32 receivedTest2ChangedCount;
	};

public:
	SyncTestsClass()
	{
		MonitoringStats::receivedEntityAddedCount = 0;
		MonitoringStats::receivedComponentChangedCount = 0;
		MonitoringStats::receivedEntityRemovedCount = 0;
		MonitoringStats::receivedTest0ChangedCount = 0;
		MonitoringStats::receivedTest1ChangedCount = 0;
		MonitoringStats::receivedTest2ChangedCount = 0;

		constexpr bool isMT = false;

		auto stream = make_unique<EntitiesStream>();
		auto manager = SystemsManager::New(isMT, Log);
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
	}

    struct TestComponent0 : Component<TestComponent0>
    {
        ui32 value;
    };

    struct TestComponent1 : Component<TestComponent1>
    {
        ui32 value;
    };

    struct TestComponent2 : NonUniqueComponent<TestComponent2>
    {
        ui32 value;
    };

    // cannot run in parallel with TestIndirectSystem2
    struct TestIndirectSystem0 : IndirectSystem<TestIndirectSystem0>
    {
		void Accept(Array<TestComponent0> &);
		using BaseIndirectSystem::ProcessMessages;

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entry : stream)
            {
                _entities.insert(entry.entityID);
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entry : stream)
            {
                _entities.erase(entry);
            }
        }

        virtual void Update(Environment &env) override
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

    private:
        std::set<EntityID> _entities{};
    };

    struct TestIndirectSystem1 : IndirectSystem<TestIndirectSystem1>
    {
		void Accept(const Array<TestComponent0> &, const Array<TestComponent1> &, const NonUnique<TestComponent2> *);
		using BaseIndirectSystem::ProcessMessages;

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entry : stream)
            {
                _entities.push_back({stream.Archetype(), entry.entityID});
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {}

        virtual void Update(Environment &env) override
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

    private:
        vector<pair<Archetype, EntityID>> _entities{};
        bool _isFirstUpdate = true;
    };

    // cannot run in parallel with TestIndirectSystem0
    struct TestIndirectSystem2 : IndirectSystem<TestIndirectSystem2>
    {
		void Accept(Array<TestComponent0> &);
		using BaseIndirectSystem::ProcessMessages;

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {}

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {}

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {}

        virtual void Update(Environment &env) override
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

    private:
        ui32 _entitiesToAdd = 100;
    };

    struct MonitoringSystem : IndirectSystem<MonitoringSystem>
    {
		void Accept();
		using BaseIndirectSystem::ProcessMessages;

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entry : stream)
            {
                ++MonitoringStats::receivedEntityAddedCount;

                for (auto &component : entry.components)
                {
                    if (component.type == TestComponent0::GetTypeId())
                    {
                        auto casted = component.Cast<TestComponent0>();
                        ASSUME(casted.value == 0 || casted.value == 10);
                    }
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
			++MonitoringStats::receivedComponentChangedCount;

			for (auto entry : stream.Enumerate<TestComponent0>())
			{
				++MonitoringStats::receivedTest0ChangedCount;
				ASSUME(entry.component.value == 1);
			}
			for (auto entry : stream.Enumerate<TestComponent1>())
			{
				++MonitoringStats::receivedTest1ChangedCount;
			}
			for (auto entry : stream.Enumerate<TestComponent2>())
			{
				++MonitoringStats::receivedTest2ChangedCount;
			}

			ASSUME(stream.Type() == TestComponent0::GetTypeId() || stream.Type() == TestComponent1::GetTypeId() || stream.Type() == TestComponent2::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entry : stream)
            {
                ++MonitoringStats::receivedEntityRemovedCount;
            }
        }

        virtual void Update(Environment &env) override
        {}
    };

    static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
    {
		stream.HintTotal(100);

        for (uiw index = 0; index < 100; ++index)
        {
            EntitiesStream::EntityData entity;

            if (index < 75)
            {
                TestComponent0 c;
                c.value = 0;
                entity.AddComponent(c);
            }

            if (index < 50)
            {
                TestComponent1 c;
                c.value = 1;
                entity.AddComponent(c);
            }

            if (index < 25)
            {
                TestComponent2 c;
                c.value = 2;
                entity.AddComponent(c);
            }

            if (index < 10)
            {
                TestComponent2 c;
                c.value = 3;
                entity.AddComponent(c);
            }

            stream.AddEntity(entityIdGenerator.Generate(), move(entity));
        }
    }

    static void PrintStreamInfo(IEntitiesStream &stream, bool isFirstPass)
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

        Log->Info("", "finished checking stream, pass %s\n", (isFirstPass ? "first" : "second"));

		Log->Info("", "ECS info:\n");
		Log->Info("", "  entities: %u\n", entitiesCount);
		Log->Info("", "  test0 components: %u\n", test0Count);
		Log->Info("", "  test1 components: %u\n", test1Count);
		Log->Info("", "  test2 components: %u\n", test2Count);
		Log->Info("", "\n");
    }
};

void SyncTests()
{
    StdLib::Initialization::Initialize({});
	SyncTestsClass test;
}