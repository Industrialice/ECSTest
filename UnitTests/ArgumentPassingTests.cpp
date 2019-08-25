#include "PreHeader.hpp"

using namespace ECSTest;

class ArgumentPassingTestsClass
{
	static constexpr bool IsMTECS = false;
	static constexpr ui32 EntitiesToTest = 2500;
	static inline std::atomic<bool> IsSystem0Visited;
	static inline std::atomic<bool> IsSystem1Visited;
	static inline std::atomic<bool> IsSystem2Visisted;

public:
	ArgumentPassingTestsClass()
	{
		IsSystem0Visited = false;
		IsSystem1Visited = false;
		IsSystem2Visisted = false;

		auto idGenerator = EntityIDGenerator{};
		auto manager = SystemsManager::New(IsMTECS, Log);
		auto stream = make_unique<EntitiesStream>();

		auto before = TimeMoment::Now();
		GenerateScene(idGenerator, *manager, *stream);
		auto after = TimeMoment::Now();
		Log->Info("", "Generating scene took %.2lfs\n", (after - before).ToSec());

		auto pipeline = manager->CreatePipeline(nullopt, false);

		manager->Register<System0>(pipeline);
		manager->Register<System1>(pipeline);
		manager->Register<System2>(pipeline);
		manager->Register<System3>(pipeline);
		manager->Register<System4>(pipeline);
		manager->Register<System5>(pipeline);
		manager->Register<System6>(pipeline);
		manager->Register<System7>(pipeline);

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

	struct ComponentBase
	{
		array<TypeId, 16> types{};
		uiw typesCount = 0;

		void AddType(TypeId type)
		{
			ASSUME(typesCount < 15);
			types[typesCount++] = type;
		}

		bool Contains(TypeId type) const
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

	struct Component0 : Component<Component0>, ComponentBase
	{
	};

	struct Component1 : Component<Component1>, ComponentBase
	{
	};

	struct Component2 : NonUniqueComponent<Component2>, ComponentBase
	{
	};

	struct Tag0 : TagComponent<Tag0> {};
	struct Tag1 : TagComponent<Tag1> {};
	struct Tag2 : TagComponent<Tag2> {};
	struct Tag3 : TagComponent<Tag3> {};

	struct StaticTag : TagComponent<StaticTag> {};
	struct StaticPositionTag : TagComponent<StaticPositionTag> {};

	struct System0 : DirectSystem<System0>
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

	struct System1 : DirectSystem<System1>
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

	struct System2 : DirectSystem<System2>
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

	struct System3 : DirectSystem<System3>
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

	struct System4 : DirectSystem<System4>
	{
		void Accept(RequiredComponent<Tag0, Tag1>, SubtractiveComponent<Tag2, Tag3>, Environment &env, const Array<Component0> &c0, const Array<Component1> *c1, const Array<EntityID> &ids, const NonUnique<Component2> *c2, RequiredComponentAny<Component1, Component2>)
		{
			for (uiw index = 0; index < c0.size(); ++index)
			{
				auto c0Data = c0[index];
				if (c1)
				{
					auto c1Data = c1->at(index);
				}
				if (c2)
				{
					auto c2Components = c2->at(index);
					for (auto c2Data : c2Components)
					{
					}
				}
			}

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

	struct System5 : IndirectSystem<System5>
	{
		using BaseIndirectSystem::ProcessMessages;

		void Accept(RequiredComponent<Tag0, Tag1>, SubtractiveComponent<Tag2, Tag3>, OptionalComponent<StaticTag, StaticPositionTag>, const Array<Component0> &c0, const Array<Component1> *c1, const NonUnique<Component2> *c2, RequiredComponentAny<Component1, Component2>) {}

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

		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
		{
		}

		std::set<EntityID> _staticEntities{}, _staticPositionEntities{};
	};

	struct System6 : IndirectSystem<System6>
	{
		using BaseIndirectSystem::ProcessMessages;

		void Accept() {}

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

		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
		{
		}

		vector<EntityID> _staticEntities{}, _staticPositionEntities{};
	};

	struct System7 : IndirectSystem<System7>
	{
		using BaseIndirectSystem::ProcessMessages;

		void Accept(const Array<Component1> &, SubtractiveComponent<Component0>) {}

		virtual void Update(Environment &env) override
		{
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
		{
			for (const auto &entry : stream)
			{
				for (const auto &c : entry.components)
				{
					ASSUME(c.type != Component0::GetTypeId());
				}
				auto it = _addedEntities.insert(entry.entityID);
				ASSUME(it.second);
			}
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
		{
			ASSUME(stream.Type() == Component1::GetTypeId());
			for (const auto &entry : stream)
			{
				for (const auto &c : entry.components)
				{
					ASSUME(c.type != Component0::GetTypeId());
				}
				auto it = _addedEntities.insert(entry.entityID);
				ASSUME(it.second);
			}
		}

		virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
		{
			ASSUME(stream.Type() == Component1::GetTypeId());
			for (const auto &entry : stream.Enumerate<Component1>())
			{
				uiw removed = _addedEntities.erase(entry.entityID);
				ASSUME(removed == 1);
			}
		}

		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
		{
			ASSUME(stream.Type() == Component1::GetTypeId());
			for (const auto &entry : stream.Enumerate<Component1>())
			{
				auto it = _addedEntities.find(entry.entityID);
				ASSUME(it != _addedEntities.end());
			}
		}

		std::set<EntityID> _addedEntities{};
	};

	static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
	{
		stream.HintTotal(EntitiesToTest);

		using tp = pair<TypeId, string_view>;
		array<tp, 7> types =
		{
			tp{Component0::GetTypeId(), NameOfType<Component0, true>.data()},
			tp{Component1::GetTypeId(), NameOfType<Component1, true>.data()},
			tp{Component2::GetTypeId(), NameOfType<Component2, true>.data()},
			tp{Tag0::GetTypeId(), NameOfType<Tag0, true>.data()},
			tp{Tag1::GetTypeId(), NameOfType<Tag1, true>.data()},
			tp{Tag2::GetTypeId(), NameOfType<Tag2, true>.data()},
			tp{Tag3::GetTypeId(), NameOfType<Tag3, true>.data()}
		};
		for (auto &[type, name] : types)
		{
			uiw found = name.find_last_of(':');
			if (found != string_view::npos)
			{
				name = name.substr(found + 1);
			}
		}

		vector<tp> left, generatedTypes;

		auto generate = [](TypeId type, EntitiesStream::EntityData &entity, Array<tp> generatedTypes)
		{
			if (type == Component0::GetTypeId() || type == Component1::GetTypeId() || type == Component2::GetTypeId())
			{
				ComponentBase base;
				for (auto &[generatedType, nameOfType] : generatedTypes)
				{
					base.AddType(generatedType);
				}
				if (type == Component0::GetTypeId())
				{
					Component0 component;
					static_cast<ComponentBase &>(component) = base;
					entity.AddComponent(component);
				}
				else if (type == Component1::GetTypeId())
				{
					Component1 component;
					static_cast<ComponentBase &>(component) = base;
					entity.AddComponent(component);
				}
				else
				{
					Component2 component;
					static_cast<ComponentBase &>(component) = base;
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
			uiw count = static_cast<uiw>(rand()) % types.size();
			EntitiesStream::EntityData entity;
			string typeNames;
			for (; count; --count)
			{
				iw typeIndex = rand() % left.size();
				auto [type, nameOfType] = left[typeIndex];
				left.erase(left.begin() + typeIndex);
				generatedTypes.push_back({type, nameOfType});
				if (!typeNames.empty())
				{
					typeNames += "_";
				}
				typeNames += nameOfType;
			}
			for (auto &[type, nameOfType] : generatedTypes)
			{
				generate(type, entity, ToArray(generatedTypes));
			}
			EntityID id = entityIdGenerator.Generate();
			id.DebugName(typeNames);
			stream.AddEntity(id, move(entity));
		}
	}
};

void ArgumentPassingTests()
{
	StdLib::Initialization::Initialize({});

	ArgumentPassingTestsClass test;
}