#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
	struct ComponentArtist : NonUniqueComponent<ComponentArtist>
	{
		enum class Areas : ui8
		{
			Undefined,
			TwoD,
			ThreeD,
			Concept
		};

		Areas area;
	};
	static_assert(sizeof(ComponentArtist) == 1);

	struct ComponentFirstName : Component<ComponentFirstName>
	{
		array<char, 32> name;
	};
	static_assert(sizeof(ComponentFirstName) == 32);

	struct ComponentLastName : Component<ComponentLastName>
	{
		array<char, 32> name;
	};
	static_assert(sizeof(ComponentLastName) == 32);

	struct ComponentDateOfBirth : Component<ComponentDateOfBirth>
	{
		ui32 dateOfBirth; // days since 01/01/2000
	};
	static_assert(sizeof(ComponentDateOfBirth) == 4);

	struct ComponentSpouse : Component<ComponentSpouse>
	{
		EntityID spouse;
		ui32 dateOfMarriage; // days since 01/01/2000
	};
	static_assert(sizeof(ComponentSpouse) == sizeof(EntityID) + 4);

	struct ComponentCompany : Component<ComponentCompany>
	{
		array<char, 32> name;
	};
	static_assert(sizeof(ComponentCompany) == 32);

	struct EmptyComponent : Component<EmptyComponent>
	{};
	static_assert(sizeof(EmptyComponent) == 1);

	struct ComponentProgrammer : NonUniqueComponent<ComponentProgrammer>
	{
		enum class Languages : ui8
		{
			Undefined,
			CPP,
			CS,
			C,
			PHP,
			JS,
			Java,
			Python
		} language;

		enum class SkillLevel : ui8
		{
			Junior, Middle, Senior
		} skillLevel;
	};
	static_assert(sizeof(ComponentProgrammer) == 2);

	struct ComponentEmployee : Component<ComponentEmployee>
	{
		EntityID employer;
	};
	static_assert(sizeof(ComponentEmployee) == sizeof(EntityID));

	struct ComponentDesigner : NonUniqueComponent<ComponentDesigner>
	{
		enum class Areas : ui8
		{
			Undefined,
			Level,
			UXUI
		};

		Areas area;
	};
	static_assert(sizeof(ComponentDesigner) == 1);

	struct ComponentGender : Component<ComponentGender>
	{
		bool isMale;
	};
	static_assert(sizeof(ComponentGender) == sizeof(bool));

	struct TagTest0 : TagComponent<TagTest0> {};
	struct TagTest1 : TagComponent<TagTest1> {};
	struct TagTest2 : TagComponent<TagTest2> {};
	struct TagTest3 : TagComponent<TagTest3> {};
	struct TagTest4 : TagComponent<TagTest4> {};
	struct TagTest5 : TagComponent<TagTest5> {};

	static_assert(sizeof(TagTest0) == 1);

	static void ArchetypeTests(bool isSuppressLogs)
	{
		TypeId types0[] = {ComponentArtist::GetTypeId(), ComponentArtist::GetTypeId()};
		auto shor = Archetype::Create<TypeId>(ToArray(types0));

		TypeId types1[] = {ComponentArtist::GetTypeId()};
		auto shor2 = Archetype::Create<TypeId>(ToArray(types1));

		ASSUME(shor == shor2);

		using typeId = pair<TypeId, ComponentID>;
		typeId types2[] = {{ComponentArtist::GetTypeId(), ComponentID(0)}, {ComponentArtist::GetTypeId(), ComponentID(1)}};
		auto arc = ArchetypeFull::Create<typeId, typeId, &typeId::first, &typeId::second>(ToArray(types2));

		typeId types3[] = {{ComponentArtist::GetTypeId(), ComponentID(0)}, {ComponentArtist::GetTypeId(), ComponentID(1)}, {ComponentArtist::GetTypeId(), ComponentID(2)}};
		auto arc2 = ArchetypeFull::Create<typeId, typeId, &typeId::first, &typeId::second>(ToArray(types3));
		ASSUME(arc != arc2);

		shor = arc.ToShort();
		shor2 = arc2.ToShort();
		ASSUME(shor == shor2);

		shor = Archetype::Create<typeId, typeId, &typeId::first>(ToArray(types2));
		shor2 = Archetype::Create<typeId, typeId, &typeId::first>(ToArray(types3));
		ASSUME(shor == shor2);

		if (!isSuppressLogs)
		{
			Log->Info("", "finished archetype tests\n");
		}
	}

	template <uiw size> static Archetype GenerateArchetype(const array<TypeId, size> &source)
	{
		return Archetype::Create<TypeId>(ToArray(source));
	}

	template <uiw size> static vector<TypeId> ToTypes(const array<TypeId, size> &source)
	{
		vector<TypeId> result;
		for (auto &req : source)
		{
			result.push_back(req);
		}
		return result;
	}

	template <uiw size> static vector<ArchetypeDefiningRequirement> ToRequired(const array<System::ComponentRequest, size> &source)
	{
		vector<ArchetypeDefiningRequirement> result;
		for (auto &req : source)
		{
			if (req.requirement != RequirementForComponent::OptionalWithData)
			{
				result.push_back({req.type, 0, req.requirement});
			}
		}
		for (uiw index = 0; index < result.size(); ++index)
		{
			result[index].group = static_cast<ui32>(index);
		}
		return result;
	}

	template <uiw size> static bool IsReflectedEqual(Array<const TypeId> reflected, const array<TypeId, size> &entity)
	{
		vector<TypeId> reflectedSorted = {reflected.begin(), reflected.end()};
		std::sort(reflectedSorted.begin(), reflectedSorted.end());

		vector<TypeId> entitySorted = {entity.begin(), entity.end()};
		std::sort(entitySorted.begin(), entitySorted.end());

		return reflectedSorted.size() == entitySorted.size() && std::equal(entitySorted.begin(), entitySorted.end(), reflectedSorted.begin());
	}

	static void ReflectorTests(bool isSuppressLogs)
	{
		array<TypeId, 5> ent0 =
		{
			ComponentArtist::GetTypeId(),
			ComponentFirstName::GetTypeId(),
			ComponentLastName::GetTypeId(),
			ComponentDateOfBirth::GetTypeId(),
			ComponentSpouse::GetTypeId()
		};
		auto arch0 = GenerateArchetype(Funcs::SortCompileTime(ent0));

		array<TypeId, 3> ent1 =
		{
			ComponentCompany::GetTypeId(),
			ComponentProgrammer::GetTypeId(),
			ComponentArtist::GetTypeId()
		};
		auto arch1 = GenerateArchetype(Funcs::SortCompileTime(ent1));

		// same as ent1, but different order
		array<TypeId, 3> ent2 =
		{
			ComponentProgrammer::GetTypeId(),
			ComponentCompany::GetTypeId(),
			ComponentArtist::GetTypeId()
		};
		auto arch2 = GenerateArchetype(Funcs::SortCompileTime(ent2));
		ASSUME(arch1 == arch2);

		array<TypeId, 5> ent3 =
		{
			ComponentSpouse::GetTypeId(),
			ComponentLastName::GetTypeId(),
			ComponentFirstName::GetTypeId(),
			ComponentEmployee::GetTypeId(),
			ComponentDesigner::GetTypeId()
		};
		auto arch3 = GenerateArchetype(Funcs::SortCompileTime(ent3));

		array<TypeId, 4> ent4 =
		{
			ComponentProgrammer::GetTypeId(),
			ComponentDesigner::GetTypeId(),
			ComponentSpouse::GetTypeId(),
			ComponentGender::GetTypeId()
		};
		auto arch4 = GenerateArchetype(Funcs::SortCompileTime(ent4));

		array<TypeId, 5> ent5 =
		{
			ComponentProgrammer::GetTypeId(),
			ComponentDesigner::GetTypeId(),
			ComponentSpouse::GetTypeId(),
			ComponentGender::GetTypeId(),
			ComponentCompany::GetTypeId()
		};
		auto arch5 = GenerateArchetype(Funcs::SortCompileTime(ent5));

		auto archEmpty = GenerateArchetype(array<TypeId, 0>{});

		// ent0 fits
		array<System::ComponentRequest, 3> req0 =
		{
			System::ComponentRequest{ComponentArtist::GetTypeId(), false, RequirementForComponent::Required},
			System::ComponentRequest{ComponentDateOfBirth::GetTypeId(), false, RequirementForComponent::OptionalWithData},
			System::ComponentRequest{ComponentCompany::GetTypeId(), false, RequirementForComponent::Subtractive},
		};
		req0 = Funcs::SortCompileTime(req0);

		// no entities fit
		array<System::ComponentRequest, 3> req1 =
		{
			System::ComponentRequest{ComponentProgrammer::GetTypeId(), false, RequirementForComponent::Required},
			System::ComponentRequest{ComponentDateOfBirth::GetTypeId(), false, RequirementForComponent::RequiredWithData},
			System::ComponentRequest{ComponentFirstName::GetTypeId(), false, RequirementForComponent::Required},
		};
		req1 = Funcs::SortCompileTime(req1);

		// ent3, ent4 fit
		array<System::ComponentRequest, 3> req2 =
		{
			System::ComponentRequest{ComponentCompany::GetTypeId(), false, RequirementForComponent::Subtractive},
			System::ComponentRequest{ComponentDesigner::GetTypeId(), false, RequirementForComponent::Required},
			System::ComponentRequest{ComponentSpouse::GetTypeId(), false, RequirementForComponent::RequiredWithData},
		};
		req2 = Funcs::SortCompileTime(req2);

		ArchetypeReflector reflector;
		reflector.AddToLibrary(arch0, ToTypes(Funcs::SortCompileTime(ent0)));
		reflector.AddToLibrary(arch1, ToTypes(Funcs::SortCompileTime(ent1)));
		reflector.AddToLibrary(archEmpty, ToTypes(array<TypeId, 0>{}));

		auto reflected = reflector.Reflect(arch0);
		ASSUME(IsReflectedEqual(reflected, Funcs::SortCompileTime(ent0)));

		reflected = reflector.Reflect(arch1);
		ASSUME(IsReflectedEqual(reflected, Funcs::SortCompileTime(ent1)));

		reflected = reflector.Reflect(archEmpty);
		ASSUME(IsReflectedEqual(reflected, array<TypeId, 0>{}));

		reflector.StartTrackingMatchingArchetypes(0, ToArray(ToRequired(req0)));
		reflector.StartTrackingMatchingArchetypes(1, ToArray(ToRequired(req1)));
		reflector.StartTrackingMatchingArchetypes(2, ToArray(ToRequired(req2)));

		auto match = reflector.FindMatchingArchetypes(0);
		ASSUME(match.size() == 1);
		ASSUME(match[0] == arch0);

		match = reflector.FindMatchingArchetypes(1);
		ASSUME(match.size() == 0);

		match = reflector.FindMatchingArchetypes(2);
		ASSUME(match.size() == 0);

		reflector.AddToLibrary(arch2, ToTypes(Funcs::SortCompileTime(ent2)));
		reflector.AddToLibrary(arch3, ToTypes(Funcs::SortCompileTime(ent3)));
		reflector.AddToLibrary(arch4, ToTypes(Funcs::SortCompileTime(ent4)));
		reflector.AddToLibrary(arch5, ToTypes(Funcs::SortCompileTime(ent5)));

		match = reflector.FindMatchingArchetypes(0);
		ASSUME(match.size() == 1);
		ASSUME(match[0] == arch0);

		match = reflector.FindMatchingArchetypes(1);
		ASSUME(match.size() == 0);

		match = reflector.FindMatchingArchetypes(2);
		ASSUME(match.size() == 2);
		ASSUME(match[0] != match[1]);
		ASSUME(match[0] == arch3 || match[0] == arch4);

		reflected = reflector.Reflect(arch0);
		ASSUME(IsReflectedEqual(reflected, ent0));

		reflected = reflector.Reflect(arch1);
		ASSUME(IsReflectedEqual(reflected, ent1));

		reflected = reflector.Reflect(arch2);
		ASSUME(IsReflectedEqual(reflected, ent2));

		reflected = reflector.Reflect(arch3);
		ASSUME(IsReflectedEqual(reflected, ent3));

		reflected = reflector.Reflect(arch4);
		ASSUME(IsReflectedEqual(reflected, ent4));

		reflected = reflector.Reflect(arch5);
		ASSUME(IsReflectedEqual(reflected, ent5));

		if (!isSuppressLogs)
		{
			Log->Info("", "finished archetype reflector tests\n");
		}
	}

	struct TestSystem : DirectSystem<TestSystem>
	{
		void Accept(NonUnique<ComponentArtist> &, RequiredComponent<TagTest0, TagTest1>, const NonUnique<ComponentProgrammer> &, Array<ComponentSpouse> &, Environment &env, const Array<EntityID> &, const Array<ComponentCompany> &, Array<ComponentEmployee> *, const Array<ComponentGender> *, RequiredComponent<ComponentDateOfBirth>, SubtractiveComponent<ComponentDesigner>)
		{}
	};

	struct TestSystem2 : DirectSystem<TestSystem2>
	{
		void Accept(RequiredComponent<TagTest0, TagTest1, TagTest2>, SubtractiveComponent<TagTest3, TagTest4, TagTest5>)
		{}
	};

	struct TestSystem3 : DirectSystem<TestSystem3>
	{
		void Accept()
		{}
	};

	struct TestSystem4 : DirectSystem<TestSystem4>
	{
		void Accept(Array<ComponentSpouse> &, const Array<ComponentCompany> &, Array<ComponentEmployee> *, const Array<ComponentGender> *, RequiredComponentAny<ComponentEmployee, ComponentGender>, OptionalComponent<ComponentDateOfBirth, ComponentDesigner>)
		{}
	};

	template <typename... Types> constexpr array<TypeId, sizeof...(Types)> MakeArray()
	{
		return {Types::GetTypeId()...};
	}

	struct TypeAndGroup
	{
		TypeId type = 0;
		ui32 group = 0;
	};

	static void ArgumentPropertiesTests()
	{
#ifdef _MSC_VER /* doesn't compile with Clang */
		auto matches = [](const Array<const System::ComponentRequest> &left, auto right, bool sort = true) constexpr -> bool
		{
			if (sort)
			{
				right = Funcs::SortCompileTime(right);
			}
			if (left.size() != right.size())
			{
				return false;
			}
			for (uiw index = 0; index < left.size(); ++index)
			{
				if (left[index].type != right[index])
				{
					return false;
				}
			}
			return true;
		};

		auto testArchetypeDefining = [](const Array<const ArchetypeDefiningRequirement> &left, auto regular, auto anyGroups) constexpr -> bool
		{
			regular = Funcs::SortCompileTime(regular);
			array<TypeAndGroup, regular.size() + anyGroups.size()> result{};
			
			if (left.size() != result.size())
			{
				return false;
			}
			
			for (uiw index = 0; index < regular.size(); ++index)
			{
				result[index] = {regular[index], static_cast<ui32>(index)};
			}
			for (uiw index = regular.size(), sourceIndex = 0; sourceIndex < anyGroups.size(); ++index, ++sourceIndex)
			{
				result[index] = {anyGroups[sourceIndex].type, anyGroups[sourceIndex].group + static_cast<ui32>(regular.size())};
			}
			
			for (uiw index = 0; index < result.size(); ++index)
			{
				if (result[index].type != left[index].type)
				{
					return false;
				}
				if (result[index].group != left[index].group)
				{
					return false;
				}
			}
			
			return true;
		};

		constexpr auto testSystemRequestsTuple = TestSystem::AcquireRequestedComponents();
		constexpr auto testSystemRequests = _SystemAuxFuncs::ComponentsTupleToRequests(testSystemRequestsTuple);

		static_assert(matches(testSystemRequests.requiredWithoutData, MakeArray<TagTest0, TagTest1, ComponentDateOfBirth>()));
		static_assert(matches(testSystemRequests.requiredWithData, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany>()));
		static_assert(matches(testSystemRequests.required, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, TagTest0, TagTest1, ComponentDateOfBirth>()));
		static_assert(matches(testSystemRequests.requiredOrOptional, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, TagTest0, TagTest1, ComponentDateOfBirth, ComponentEmployee, ComponentGender>()));
		static_assert(matches(testSystemRequests.withData, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender>()));
		static_assert(matches(testSystemRequests.optional, MakeArray<>()));
		static_assert(matches(testSystemRequests.optionalWithData, MakeArray<ComponentEmployee, ComponentGender>()));
		static_assert(matches(testSystemRequests.subtractive, MakeArray<ComponentDesigner>()));
		static_assert(matches(testSystemRequests.writeAccess, MakeArray<ComponentArtist, ComponentSpouse, ComponentEmployee>()));
		static_assert(matches(testSystemRequests.all, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, TagTest0, TagTest1, ComponentDateOfBirth, ComponentEmployee, ComponentGender, ComponentDesigner>()));
		static_assert(matches(testSystemRequests.argumentPassingOrder, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender>(), false));
		static_assert(testArchetypeDefining(testSystemRequests.archetypeDefiningInfoOnly, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, TagTest0, TagTest1, ComponentDateOfBirth, ComponentDesigner>(), array<TypeAndGroup, 0>{}));
		static_assert(testSystemRequests.entityIDIndex == 4);
		static_assert(testSystemRequests.environmentIndex == 3);

		constexpr auto testSystem2RequestsTuple = TestSystem2::AcquireRequestedComponents();
		constexpr auto testSystem2Requests = _SystemAuxFuncs::ComponentsTupleToRequests(testSystem2RequestsTuple);

		static_assert(matches(testSystem2Requests.requiredWithoutData, MakeArray<TagTest0, TagTest1, TagTest2>()));
		static_assert(matches(testSystem2Requests.requiredWithData, MakeArray<>()));
		static_assert(matches(testSystem2Requests.required, MakeArray<TagTest0, TagTest1, TagTest2>()));
		static_assert(matches(testSystem2Requests.requiredOrOptional, MakeArray<TagTest0, TagTest1, TagTest2>()));
		static_assert(matches(testSystem2Requests.withData, MakeArray<>()));
		static_assert(matches(testSystem2Requests.optional, MakeArray<>()));
		static_assert(matches(testSystem2Requests.optionalWithData, MakeArray<>()));
		static_assert(matches(testSystem2Requests.subtractive, MakeArray<TagTest3, TagTest4, TagTest5>()));
		static_assert(matches(testSystem2Requests.writeAccess, MakeArray<>()));
		static_assert(matches(testSystem2Requests.all, MakeArray<TagTest0, TagTest1, TagTest2, TagTest3, TagTest4, TagTest5>()));
		static_assert(matches(testSystem2Requests.argumentPassingOrder, MakeArray<>(), false));
		static_assert(testArchetypeDefining(testSystem2Requests.archetypeDefiningInfoOnly, MakeArray<TagTest0, TagTest1, TagTest2, TagTest3, TagTest4, TagTest5>(), array<TypeAndGroup, 0>{}));
		static_assert(testSystem2Requests.entityIDIndex == nullopt);
		static_assert(testSystem2Requests.environmentIndex == nullopt);
		
		constexpr auto testSystem3RequestsTuple = TestSystem3::AcquireRequestedComponents();
		constexpr auto testSystem3Requests = _SystemAuxFuncs::ComponentsTupleToRequests(testSystem3RequestsTuple);

		static_assert(matches(testSystem3Requests.requiredWithoutData, MakeArray<>()));
		static_assert(matches(testSystem3Requests.requiredWithData, MakeArray<>()));
		static_assert(matches(testSystem3Requests.required, MakeArray<>()));
		static_assert(matches(testSystem3Requests.requiredOrOptional, MakeArray<>()));
		static_assert(matches(testSystem3Requests.withData, MakeArray<>()));
		static_assert(matches(testSystem3Requests.optional, MakeArray<>()));
		static_assert(matches(testSystem3Requests.optionalWithData, MakeArray<>()));
		static_assert(matches(testSystem3Requests.subtractive, MakeArray<>()));
		static_assert(matches(testSystem3Requests.writeAccess, MakeArray<>()));
		static_assert(matches(testSystem3Requests.all, MakeArray<>()));
		static_assert(matches(testSystem3Requests.argumentPassingOrder, MakeArray<>(), false));
		static_assert(testArchetypeDefining(testSystem3Requests.archetypeDefiningInfoOnly, MakeArray<>(), array<TypeAndGroup, 0>{}));
		static_assert(testSystem3Requests.entityIDIndex == nullopt);
		static_assert(testSystem3Requests.environmentIndex == nullopt);

		constexpr auto testSystem4RequestsTuple = TestSystem4::AcquireRequestedComponents();
		constexpr auto testSystem4Requests = _SystemAuxFuncs::ComponentsTupleToRequests(testSystem4RequestsTuple);
		
		static_assert(matches(testSystem4Requests.requiredWithoutData, MakeArray<>()));
		static_assert(matches(testSystem4Requests.requiredWithData, MakeArray<ComponentSpouse, ComponentCompany>()));
		static_assert(matches(testSystem4Requests.required, MakeArray<ComponentSpouse, ComponentCompany>()));
		static_assert(matches(testSystem4Requests.requiredOrOptional, MakeArray<ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender, ComponentDateOfBirth, ComponentDesigner>()));
		static_assert(matches(testSystem4Requests.withData, MakeArray<ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender>()));
		static_assert(matches(testSystem4Requests.optional, MakeArray<ComponentDateOfBirth, ComponentDesigner>()));
		static_assert(matches(testSystem4Requests.optionalWithData, MakeArray<ComponentEmployee, ComponentGender>()));
		static_assert(matches(testSystem4Requests.subtractive, MakeArray<>()));
		static_assert(matches(testSystem4Requests.writeAccess, MakeArray<ComponentSpouse, ComponentEmployee>()));
		static_assert(matches(testSystem4Requests.all, MakeArray<ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender, ComponentDateOfBirth, ComponentDesigner>()));
		static_assert(matches(testSystem4Requests.argumentPassingOrder, MakeArray<ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender>(), false));
		static_assert(testArchetypeDefining(testSystem4Requests.archetypeDefiningInfoOnly, MakeArray<ComponentSpouse, ComponentCompany>(), make_array(TypeAndGroup{ComponentEmployee::GetTypeId(), 0}, TypeAndGroup{ComponentGender::GetTypeId(), 0})));
		static_assert(testSystem4Requests.entityIDIndex == nullopt);
		static_assert(testSystem4Requests.environmentIndex == nullopt);
#endif
	}
}

class UnitTests
{
public:
	static void MessageBuilderTests(bool isSuppressLogs)
	{
		EntityIDGenerator gen;
		MessageBuilder builder;
		std::map<EntityID, ComponentFirstName> entityNames;
		std::map<EntityID, ComponentFirstName> entityAfterChangeNames;
		std::set<EntityID> entitiesRemoved;

		auto generateName = []
		{
			ComponentFirstName name;
			name.name.fill(0);
			uiw len = static_cast<uiw>(rand()) % 10 + 5;
			for (uiw index = 0; index < len; ++index)
			{
				name.name[index] = 'a' + rand() % 24;
			}
			return name;
		};

		for (ui32 index = 0; index < 1000; ++index)
		{
			{
				auto &componentBuilder = builder.AddEntity(gen.Generate());

				ComponentFirstName name = generateName();
				entityNames[gen.LastGenerated()] = name;

				ComponentArtist artist;
				artist.area = ComponentArtist::Areas::Concept;

				componentBuilder.AddComponent(name).AddComponent(artist).AddComponent(TagTest0{}).AddComponent(TagTest1{}).AddComponent(TagTest2{});
			}

			if (rand() % 10 == 0)
			{
				TypeId types[] =
				{
					ComponentFirstName::GetTypeId(),
					ComponentArtist::GetTypeId()
				};
				Archetype arch = Archetype::Create<TypeId>(ToArray(types));
				builder.RemoveEntity(gen.LastGenerated(), arch);
				auto[it, result] = entitiesRemoved.insert(gen.LastGenerated());
				ASSUME(result);
			}

			if (rand() % 4 == 0)
			{
				ComponentFirstName changed = generateName();

				builder.ComponentChanged(gen.LastGenerated(), changed);

				entityAfterChangeNames[gen.LastGenerated()] = changed;
			}
		}

		ui32 checked = 0;
		for (const auto &streamSource : builder.EntityAddedStreams()._data)
		{
			MessageStreamEntityAdded addedStream(streamSource.first, streamSource.second, "MessageBuilderTests");

			for (const auto &entity : addedStream)
			{
				ASSUME(entity.components.size() == 5);

				++checked;

				const auto &c0 = entity.components[0];
				const auto &c1 = entity.components[1];

				for (uiw index = 2; index < 5; ++index)
				{
					const auto &c = entity.components[index];
					ASSUME(c.type == TagTest0::GetTypeId() || c.type == TagTest1::GetTypeId() || c.type == TagTest2::GetTypeId());
					ASSUME(c.isTag == true);
					ASSUME(c.isUnique == true);
					ASSUME(c.data == nullptr);
				}

				ComponentFirstName refName = entityNames[entity.entityID];

				auto checkFirstName = [refName](const SerializedComponent &c)
				{
					const ComponentFirstName &casted = c.Cast<ComponentFirstName>();
					ASSUME(!MemOps::Compare(casted.name.data(), refName.name.data(), refName.name.size()));
				};

				checkFirstName(c0.type == ComponentFirstName::GetTypeId() ? c0 : c1);

				auto checkArtist = [](const SerializedComponent &c)
				{
					const ComponentArtist &casted = c.Cast<ComponentArtist>();
					ASSUME(casted.area == ComponentArtist::Areas::Concept);
				};

				checkArtist(c0.type == ComponentArtist::GetTypeId() ? c0 : c1);
			}
		}
		ASSUME(checked == entityNames.size());

		checked = 0;
		for (const auto &streamSource : builder.EntityRemovedStreams()._data)
		{
			MessageStreamEntityRemoved removedStream(streamSource.first, streamSource.second, "MessageBuilderTests");

			for (const auto &id : removedStream)
			{
				ASSUME(entitiesRemoved.find(id) != entitiesRemoved.end());
				++checked;
			}
		}
		ASSUME(checked == entitiesRemoved.size());

		checked = 0;
		for (const auto &[type, streamSource] : builder.ComponentChangedStreams()._data)
		{
			MessageStreamComponentChanged changed(streamSource.second, streamSource.first, "MessageBuilderTests");

			for (auto component : changed.Enumerate<ComponentFirstName>())
			{
				++checked;
				auto &casted = component.component;
				ASSUME(!MemOps::Compare(entityAfterChangeNames[component.entityID].name.data(), casted.name.data(), casted.name.size()));
			}
		}
		ASSUME(checked == entityAfterChangeNames.size());

		if (!isSuppressLogs)
		{
			Log->Info("", "finished message builder tests\n");
		}
	}
};

void PerformUnitTests(bool isSuppressLogs)
{
    Initialization::Initialize({});

    ArchetypeTests(isSuppressLogs);
    ReflectorTests(isSuppressLogs);
    UnitTests::MessageBuilderTests(isSuppressLogs);
	ArgumentPropertiesTests();
}