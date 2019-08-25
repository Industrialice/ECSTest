#include "PreHeader.hpp"

using namespace ECSTest;

struct SimpleOrderValues
{
	ui32 ProducerUpdatedTimes{};
	ui32 ConsumerUpdatedTimes{};
};

class SimpleOrderTestsClass
{
	static constexpr bool IsMultiThreadedECS = false;
	static constexpr ui32 WaitForExecutedFrames = 5;

	struct TestComponent : Component<TestComponent>
	{
		ui32 value;
	};

public:
	SimpleOrderTestsClass()
	{
		SimpleOrderValues args{};

		auto manager = SystemsManager::New(IsMultiThreadedECS, Log);
		EntityIDGenerator entityIdGenerator;

		auto testPipeline = manager->CreatePipeline(nullopt, false);

		manager->Register<ProducerSystem>(testPipeline, args);
		manager->Register<ConsumerSystem>(testPipeline, args);

		vector<WorkerThread> workers;
		if (IsMultiThreadedECS)
		{
			workers.resize(SystemInfo::LogicalCPUCores());
		}

		manager->Start(move(entityIdGenerator), move(workers));

		for (;;)
		{
			auto info = manager->GetPipelineInfo(testPipeline);
			if (info.executedTimes > WaitForExecutedFrames)
			{
				break;
			}
			std::this_thread::yield();
		}

		manager->Stop(true);

		ASSUME(args.ConsumerUpdatedTimes == args.ProducerUpdatedTimes);
		ASSUME(args.ConsumerUpdatedTimes > 0);
	}

	struct ProducerSystem : public IndirectSystem<ProducerSystem>
	{
		bool _isUpdated = false;
		SimpleOrderValues &_args;

		ProducerSystem(SimpleOrderValues &args) : _args(args) {}

		void Accept(Array<TestComponent> &) {}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) override
		{
			SOFTBREAK;
		}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) override
		{
			SOFTBREAK;
		}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
		{
			SOFTBREAK;
		}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) override
		{
			SOFTBREAK;
		}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) override
		{
			SOFTBREAK;
		}
		virtual void Update(Environment &env) override
		{
			_args.ProducerUpdatedTimes++;

			if (_isUpdated)
			{
				return;
			}

			auto id = env.messageBuilder.AddEntity();
			env.messageBuilder.AddComponent(id, TestComponent{.value = 1});
			env.messageBuilder.ComponentChanged(id, TestComponent{.value = 2});

			_isUpdated = true;
		}
	};

	struct ConsumerSystem : public IndirectSystem<ConsumerSystem>
	{
		SimpleOrderValues &_args;
		ConsumerSystem(SimpleOrderValues &args) : _args(args) {}

		void Accept(const Array<TestComponent> &) {}

		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) override
		{
			for (auto &entry : stream)
			{
				if (entry.entityID == EntityID(0, 0))
				{
					TestComponent testComponent = entry.GetComponent<TestComponent>();
					ASSUME(entry.components.size() == 1);
					ASSUME(testComponent.value == 1);
				}
			}
		}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) override
		{
			SOFTBREAK;
		}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
		{
			ASSUME(stream.Type() == TestComponent::GetTypeId());
			for (const auto &entry : stream.Enumerate<TestComponent>())
			{
				ASSUME(entry.component.value == 2);
			}
		}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) override
		{
			SOFTBREAK;
		}
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) override
		{
			SOFTBREAK;
		}
		virtual void Update(Environment &env) override
		{
			_args.ConsumerUpdatedTimes++;
		}
	};
};

void SimpleOrderTests()
{
	StdLib::Initialization::Initialize({});
	SimpleOrderTestsClass test;
}