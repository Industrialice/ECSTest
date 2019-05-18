#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
	constexpr bool IsMTECS = false;
	constexpr ui32 EntitiesToTest = 2500;
	std::atomic<bool> IsSystem0Visited = false;
	std::atomic<bool> IsSystem1Visited = false;
	std::atomic<bool> IsSystem2Visisted = false;

	struct ComponentBase
	{
		array<StableTypeId, 16> types{};
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

	TAG_COMPONENT(StaticTag);
	TAG_COMPONENT(StaticPositionTag);

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
				ASSUME(component2.ids[index] && component2.ids[index] <= env.componentIdGenerator.LastGenerated());
				ASSUME(ids[index] && ids[index] <= env.entityIdGenerator.LastGenerated());
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
					ASSUME(component0[index] == component1->at(index));
				}
				ASSUME(component0[index].Contains(Component0::GetTypeId()));
				ASSUME(!component0[index].Contains(Component2::GetTypeId()));
				ASSUME(!component0[index].Contains(Tag0::GetTypeId()));
			}
		}
	};

	DIRECT_SYSTEM(System3)
	{
		void Accept(const Array<Component0> *component0, const Array<Component1> *component1, RequiredComponentAny<Component0, Component1>)
		{
			ASSUME(component0 || component1);

			auto test = [](const auto *left, const auto *right)
			{
				auto leftId = std::remove_pointer_t<decltype(left)>::ItemType::GetTypeId();
				auto rightId = std::remove_pointer_t<decltype(right)>::ItemType::GetTypeId();

				for (uiw index = 0; index < left->size(); ++index)
				{
					ASSUME(left->at(index).Contains(leftId));

					if (right)
					{
						ASSUME(left->at(index).Contains(rightId));
						ASSUME(right->at(index).Contains(leftId));
						ASSUME(right->at(index).Contains(rightId));
					}
					else
					{
						ASSUME(false == left->at(index).Contains(rightId));
					}
				}
			};

			if (component0)
			{
				test(component0, component1);
			}
			else
			{
				test(component1, component0);
			}
		}
	};

	DIRECT_SYSTEM(System4)
	{
		void Accept(RequiredComponent<Tag0, Tag1>, SubtractiveComponent<Tag2, Tag3>, Environment &env, const Array<Component0> &c0, const Array<Component1> *c1, const Array<EntityID> &ids, const NonUnique<Component2> *c2, RequiredComponentAny<Component1, Component2>)
		{
			for (uiw index = 0; index < c0.size(); ++index)
			{
				const Component0 &c = c0[index];

				ASSUME(c.Contains(Tag0::GetTypeId()));
				ASSUME(c.Contains(Tag1::GetTypeId()));
				ASSUME(!c.Contains(Tag2::GetTypeId()));
				ASSUME(!c.Contains(Tag3::GetTypeId()));
				ASSUME(c.Contains(Component0::GetTypeId()));
				if (c1)
				{
					ASSUME(c.Contains(Component1::GetTypeId()));
				}
				if (c2)
				{
					ASSUME(c.Contains(Component2::GetTypeId()));
				}
				ASSUME(c1 || c2);
			}
		}
	};

	INDIRECT_SYSTEM(System5)
	{
		using IndirectSystem::ProcessMessages;

		void Accept(RequiredComponent<Tag0, Tag1>, SubtractiveComponent<Tag2, Tag3>, OptionalComponent<StaticTag, StaticPositionTag>, const Array<Component0> &c0, const Array<Component1> *c1, const NonUnique<Component2> *c2, RequiredComponentAny<Component1, Component2>);

		virtual void Update(Environment &env) override
		{
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
		{
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
		{
			for (auto &entry : stream)
			{
				if (stream.Type() == StaticTag::GetTypeId())
				{
					ASSUME(entry.Find<StaticTag>());
					_staticEntities.insert(entry.entityID);
				}
				else if (stream.Type() == StaticPositionTag::GetTypeId())
				{
					ASSUME(entry.Find<StaticPositionTag>());
					_staticPositionEntities.insert(entry.entityID);
				}
				else
				{
					SOFTBREAK;
				}
			}
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
		{
			for (const auto &entry : stream.Enumerate<StaticTag>())
			{
				uiw count = _staticEntities.erase(entry.entityID);
				ASSUME(count == 1);
			}
			for (const auto &entry : stream.Enumerate<StaticPositionTag>())
			{
				uiw count = _staticPositionEntities.erase(entry.entityID);
				ASSUME(count == 1);
			}

			ASSUME(stream.Type() == StaticTag::GetTypeId() || stream.Type() == StaticPositionTag::GetTypeId());
		}

		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream)
		{
		}

		std::set<EntityID> _staticEntities{}, _staticPositionEntities{};
	};

	INDIRECT_SYSTEM(System6)
	{
		using IndirectSystem::ProcessMessages;

		void Accept();

		virtual void Update(Environment &env) override
		{
			for (auto id : _staticEntities)
			{
				env.messageBuilder.RemoveComponent(id, StaticTag::GetTypeId(), {});
			}
			for (auto id : _staticPositionEntities)
			{
				env.messageBuilder.RemoveComponent(id, StaticPositionTag::GetTypeId(), {});
			}
			_staticEntities.clear();
			_staticPositionEntities.clear();
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
		{
			for (auto &entry : stream)
			{
				if (rand() % 2)
				{
					if (rand() % 2)
					{
						env.messageBuilder.AddComponent(entry.entityID, StaticTag{});
						_staticEntities.push_back(entry.entityID);
					}
					else
					{
						env.messageBuilder.AddComponent(entry.entityID, StaticPositionTag{});
						_staticPositionEntities.push_back(entry.entityID);
					}
				}
			}
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
		{
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
		{
		}

		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) 
		{
		}

		vector<EntityID> _staticEntities{}, _staticPositionEntities{};
	};

	static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
	{
		stream.HintTotal(EntitiesToTest);

		static constexpr array<StableTypeId, 7> types =
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
	manager->Register<System3>(pipeline);
	manager->Register<System4>(pipeline);
	manager->Register<System5>(pipeline);
	manager->Register<System6>(pipeline);

	vector<WorkerThread> workers;
	if (IsMTECS)
	{
		workers.resize(SystemInfo::LogicalCPUCores());
	}

	manager->Start(move(idGenerator), move(workers), move(stream));

	for (;;)
	{
		auto info = manager->GetPipelineInfo(pipeline);
		if (info.executedTimes > 2)
		{
			break;
		}
		std::this_thread::yield();
	}

	manager->Stop(true);

	ASSUME(IsSystem0Visited && IsSystem1Visited && IsSystem2Visisted);
}