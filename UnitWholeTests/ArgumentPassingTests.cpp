#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
	constexpr bool IsMTECS = false;
	constexpr ui32 EntitiesToTest = 25000;
	std::atomic<bool> IsSystem0Visited = false;
	std::atomic<bool> IsSystem1Visited = false;
	std::atomic<bool> IsSystem2Visisted = false;

	struct ComponentBase
	{
		std::array<StableTypeId, 16> types{};
		uiw typesCount = 0;

		void AddType(StableTypeId type)
		{
			ASSUME(typesCount < 15);
			types[typesCount++] = type;
		}

		bool Contains(StableTypeId type) const
		{
			for (uiw index = 0; index < typesCount; ++index)
			{
				if (types[index] == type)
				{
					return true;
				}
			}
			return false;
		}

		bool operator == (const ComponentBase &other) const
		{
			if (typesCount != other.typesCount)
			{
				return false;
			}
			return !MemOps::Compare(types.data(), other.types.data(), typesCount);
		}
	};

	COMPONENT(Component0), ComponentBase
	{
	};

	COMPONENT(Component1), ComponentBase
	{
	};

	NONUNIQUE_COMPONENT(Component2), ComponentBase
	{
	};

	TAG_COMPONENT(Tag0);
	TAG_COMPONENT(Tag1);
	TAG_COMPONENT(Tag2);
	TAG_COMPONENT(Tag3);

	DIRECT_SYSTEM(System0)
	{
		void Accept(RequiredComponent<Tag0, Tag1>, SubtractiveComponent<Tag2, Tag3>, const Array<Component0> &data)
		{
			IsSystem0Visited = true;
			for (auto &entry : data)
			{
				ASSUME(entry.Contains(Tag0::GetTypeId()));
				ASSUME(entry.Contains(Tag1::GetTypeId()));
				ASSUME(!entry.Contains(Tag2::GetTypeId()));
				ASSUME(!entry.Contains(Tag3::GetTypeId()));
				ASSUME(entry.Contains(Component0::GetTypeId()));
			}
		}
	};

	DIRECT_SYSTEM(System1)
	{
		void Accept(const Array<Component0> &component0, const Array<EntityID> &ids, Array<Component1> &component1, Environment &env, const NonUnique<Component2> &component2)
		{
			IsSystem1Visited = true;
			ASSUME(component0.size() == ids.size());
			ASSUME(component0.size() == component1.size());
			ASSUME(component0.size() == component2.components.size());
			ASSUME(component0.size() == component2.ids.size());
			ASSUME(component2.stride == 1);
			for (uiw index = 0; index < component0.size(); ++index)
			{
				ASSUME(component0[index] == component1[index]);
				ASSUME(component0[index] == component2.components[index]);
				ASSUME(component2.ids[index].IsValid() && component2.ids[index] <= env.componentIdGenerator.LastGenerated());
				ASSUME(ids[index].IsValid() && ids[index] <= env.entityIdGenerator.LastGenerated());
			}
		}
	};

	DIRECT_SYSTEM(System2)
	{
		void Accept(const Array<Component0> &component0, const Array<Component1> *component1, SubtractiveComponent<Component2, Tag0>, const Array<EntityID> &ids)
		{
			IsSystem2Visisted = true;
			ASSUME(component0.size() == ids.size());
			if (component1)
			{
				ASSUME(component0.size() == component1->size());
			}
			for (uiw index = 0; index < component0.size(); ++index)
			{
				if (component1)
				{
					ASSUME(component0[index] == component1->operator[](index));
				}
				ASSUME(component0[index].Contains(Component0::GetTypeId()));
				ASSUME(!component0[index].Contains(Component2::GetTypeId()));
				ASSUME(!component0[index].Contains(Tag0::GetTypeId()));
			}
		}
	};

	DIRECT_SYSTEM(System3)
	{
		void Accept()
		{
		}
	};

	static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
	{
		stream.HintTotal(EntitiesToTest);

		static constexpr std::array<StableTypeId, 7> types =
		{
			Component0::GetTypeId(),
			Component1::GetTypeId(),
			Component2::GetTypeId(),
			Tag0::GetTypeId(),
			Tag1::GetTypeId(),
			Tag2::GetTypeId(),
			Tag3::GetTypeId()
		};

		vector<StableTypeId> left, generatedTypes;

		auto generate = [](StableTypeId type, EntitiesStream::EntityData &entity, Array<StableTypeId> generatedTypes)
		{
			if (type == Component0::GetTypeId() || type == Component1::GetTypeId() || type == Component2::GetTypeId())
			{
				ComponentBase base;
				for (StableTypeId generatedType : generatedTypes)
				{
					base.AddType(generatedType);
				}
				if (type == Component0::GetTypeId())
				{
					Component0 component;
					(ComponentBase &)component = base;
					entity.AddComponent(component);
				}
				else if (type == Component1::GetTypeId())
				{
					Component1 component;
					(ComponentBase &)component = base;
					entity.AddComponent(component);
				}
				else
				{
					Component2 component;
					(ComponentBase &)component = base;
					entity.AddComponent(component);
				}
			}
			else if (type == Tag0::GetTypeId())
			{
				entity.AddComponent(Tag0{});
			}
			else if (type == Tag1::GetTypeId())
			{
				entity.AddComponent(Tag1{});
			}
			else if (type == Tag2::GetTypeId())
			{
				entity.AddComponent(Tag2{});
			}
			else if (type == Tag3::GetTypeId())
			{
				entity.AddComponent(Tag3{});
			}
			else
			{
				SOFTBREAK;
			}
		};

		for (uiw index = 0; index < EntitiesToTest; ++index)
		{
			generatedTypes.clear();
			left.assign(types.begin(), types.end());
			uiw count = (uiw)rand() % types.size();
			EntitiesStream::EntityData entity;
			for (; count; --count)
			{
				iw typeIndex = rand() % left.size();
				StableTypeId type = left[typeIndex];
				left.erase(left.begin() + typeIndex);
				generatedTypes.push_back(type);
			}
			for (StableTypeId type : generatedTypes)
			{
				generate(type, entity, ToArray(generatedTypes));
			}
			stream.AddEntity(entityIdGenerator.Generate(), move(entity));
		}
	}
}

void ArgumentPassingTests()
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

	auto pipeline = manager->CreatePipeline(nullopt, false);

	manager->Register<System0>(pipeline);
	manager->Register<System1>(pipeline);
	manager->Register<System2>(pipeline);
	//manager->Register<System3>(pipeline);

	vector<WorkerThread> workers;
	if (IsMTECS)
	{
		workers.resize(SystemInfo::LogicalCPUCores());
	}

	manager->Start(move(idGenerator), move(workers), move(stream));

	for (;;)
	{
		auto info = manager->GetPipelineInfo(pipeline);
		if (info.executedTimes > 0)
		{
			break;
		}
		std::this_thread::yield();
	}

	manager->Stop(true);

	ASSUME(IsSystem0Visited && IsSystem1Visited && IsSystem2Visisted);
}