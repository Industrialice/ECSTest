#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
	NONUNIQUE_COMPONENT(ComponentArtist)
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

	COMPONENT(ComponentFirstName)
	{
		array<char, 32> name;
	};
	static_assert(sizeof(ComponentFirstName) == 32);

	COMPONENT(ComponentLastName)
	{
		array<char, 32> name;
	};
	static_assert(sizeof(ComponentLastName) == 32);

	COMPONENT(ComponentDateOfBirth)
	{
		ui32 dateOfBirth; // days since 01/01/2000
	};
	static_assert(sizeof(ComponentDateOfBirth) == 4);

	COMPONENT(ComponentSpouse)
	{
		EntityID spouse;
		ui32 dateOfMarriage; // days since 01/01/2000
	};
	static_assert(sizeof(ComponentSpouse) == sizeof(EntityID) + 4);

	COMPONENT(ComponentCompany)
	{
		array<char, 32> name;
	};
	static_assert(sizeof(ComponentCompany) == 32);

	COMPONENT(EmptyComponent)
	{};
	static_assert(sizeof(EmptyComponent) == 1);

	NONUNIQUE_COMPONENT(ComponentProgrammer)
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

	COMPONENT(ComponentEmployee)
	{
		EntityID employer;
	};
	static_assert(sizeof(ComponentEmployee) == sizeof(EntityID));

	NONUNIQUE_COMPONENT(ComponentDesigner)
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

	COMPONENT(ComponentGender)
	{
		bool isMale;
	};
	static_assert(sizeof(ComponentGender) == sizeof(bool));

	TAG_COMPONENT(TagTest0);
	TAG_COMPONENT(TagTest1);
	TAG_COMPONENT(TagTest2);
	TAG_COMPONENT(TagTest3);
	TAG_COMPONENT(TagTest4);
	TAG_COMPONENT(TagTest5);

	static_assert(sizeof(TagTest0) == 1);

	static void ArchetypeTests(bool isSuppressLogs)
	{
		StableTypeId types0[] = {ComponentArtist::GetTypeId(), ComponentArtist::GetTypeId()};
		auto shor = Archetype::Create<StableTypeId>(ToArray(types0));

		StableTypeId types1[] = {ComponentArtist::GetTypeId()};
		auto shor2 = Archetype::Create<StableTypeId>(ToArray(types1));

		ASSUME(shor == shor2);

		using typeId = pair<StableTypeId, ComponentID>;
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
			printf("finished archetype tests\n");
		}
	}

	template <uiw size> static Archetype GenerateArchetype(const array<StableTypeId, size> &source)
	{
		return Archetype::Create<StableTypeId>(ToArray(source));
	}

	template <uiw size> static vector<StableTypeId> ToTypes(const array<StableTypeId, size> &source)
	{
		vector<StableTypeId> result;
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
			result[index].group = (ui8)index;
		}
		return result;
	}

	template <uiw size> static bool IsReflectedEqual(Array<const StableTypeId> reflected, const array<StableTypeId, size> &entity)
	{
		vector<StableTypeId> reflectedSorted = {reflected.begin(), reflected.end()};
		std::sort(reflectedSorted.begin(), reflectedSorted.end());

		vector<StableTypeId> entitySorted = {entity.begin(), entity.end()};
		std::sort(entitySorted.begin(), entitySorted.end());

		return reflectedSorted.size() == entitySorted.size() && std::equal(entitySorted.begin(), entitySorted.end(), reflectedSorted.begin());
	}

	static void ReflectorTests(bool isSuppressLogs)
	{
		array<StableTypeId, 5> ent0 =
		{
			ComponentArtist::GetTypeId(),
			ComponentFirstName::GetTypeId(),
			ComponentLastName::GetTypeId(),
			ComponentDateOfBirth::GetTypeId(),
			ComponentSpouse::GetTypeId()
		};
		auto arch0 = GenerateArchetype(Funcs::SortCompileTime(ent0));

		array<StableTypeId, 3> ent1 =
		{
			ComponentCompany::GetTypeId(),
			ComponentProgrammer::GetTypeId(),
			ComponentArtist::GetTypeId()
		};
		auto arch1 = GenerateArchetype(Funcs::SortCompileTime(ent1));

		// same as ent1, but different order
		array<StableTypeId, 3> ent2 =
		{
			ComponentProgrammer::GetTypeId(),
			ComponentCompany::GetTypeId(),
			ComponentArtist::GetTypeId()
		};
		auto arch2 = GenerateArchetype(Funcs::SortCompileTime(ent2));
		ASSUME(arch1 == arch2);

		array<StableTypeId, 5> ent3 =
		{
			ComponentSpouse::GetTypeId(),
			ComponentLastName::GetTypeId(),
			ComponentFirstName::GetTypeId(),
			ComponentEmployee::GetTypeId(),
			ComponentDesigner::GetTypeId()
		};
		auto arch3 = GenerateArchetype(Funcs::SortCompileTime(ent3));

		array<StableTypeId, 4> ent4 =
		{
			ComponentProgrammer::GetTypeId(),
			ComponentDesigner::GetTypeId(),
			ComponentSpouse::GetTypeId(),
			ComponentGender::GetTypeId()
		};
		auto arch4 = GenerateArchetype(Funcs::SortCompileTime(ent4));

		array<StableTypeId, 5> ent5 =
		{
			ComponentProgrammer::GetTypeId(),
			ComponentDesigner::GetTypeId(),
			ComponentSpouse::GetTypeId(),
			ComponentGender::GetTypeId(),
			ComponentCompany::GetTypeId()
		};
		auto arch5 = GenerateArchetype(Funcs::SortCompileTime(ent5));

		auto archEmpty = GenerateArchetype(array<StableTypeId, 0>{});

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
		reflector.AddToLibrary(archEmpty, ToTypes(array<StableTypeId, 0>{}));

		auto reflected = reflector.Reflect(arch0);
		ASSUME(IsReflectedEqual(reflected, Funcs::SortCompileTime(ent0)));

		reflected = reflector.Reflect(arch1);
		ASSUME(IsReflectedEqual(reflected, Funcs::SortCompileTime(ent1)));

		reflected = reflector.Reflect(archEmpty);
		ASSUME(IsReflectedEqual(reflected, array<StableTypeId, 0>{}));

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
			printf("finished archetype reflector tests\n");
		}
	}

	DIRECT_SYSTEM(TestSystem)
	{
		void Accept(NonUnique<ComponentArtist> &, RequiredComponent<TagTest0, TagTest1>, const NonUnique<ComponentProgrammer> &, Array<ComponentSpouse> &, Environment &env, const Array<EntityID> &, const Array<ComponentCompany> &, Array<ComponentEmployee> *, const Array<ComponentGender> *, RequiredComponent<ComponentDateOfBirth>, SubtractiveComponent<ComponentDesigner>)
		{}
	};

	INDIRECT_SYSTEM(TestSystem2)
	{
		void Accept(RequiredComponent<TagTest0, TagTest1, TagTest2>, SubtractiveComponent<TagTest3, TagTest4, TagTest5>)
		{}
	};

	INDIRECT_SYSTEM(TestSystem3)
	{
		void Accept()
		{}
	};

	INDIRECT_SYSTEM(TestSystem4)
	{
		void Accept(Array<ComponentSpouse> &, const Array<ComponentCompany> &, Array<ComponentEmployee> *, const Array<ComponentGender> *, RequiredComponentAny<ComponentEmployee, ComponentGender>, OptionalComponent<ComponentDateOfBirth, ComponentDesigner>)
		{}
	};

	template <typename... Types> constexpr array<StableTypeId, sizeof...(Types)> MakeArray()
	{
		return {Types::GetTypeId()...};
	}

	struct TypeAndGroup
	{
		StableTypeId type = 0;
		ui32 group = 0;
	};

	static void ArgumentPropertiesTests()
	{
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
				result[index] = {regular[index], (ui32)index};
			}
			for (uiw index = regular.size(), sourceIndex = 0; sourceIndex < anyGroups.size(); ++index, ++sourceIndex)
			{
				result[index] = {anyGroups[sourceIndex].type, anyGroups[sourceIndex].group + (ui32)regular.size()};
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

		static_assert(matches(TestSystem::AcquireRequestedComponents().requiredWithoutData, MakeArray<TagTest0, TagTest1, ComponentDateOfBirth>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().requiredWithData, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().required, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, TagTest0, TagTest1, ComponentDateOfBirth>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().requiredOrOptional, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, TagTest0, TagTest1, ComponentDateOfBirth, ComponentEmployee, ComponentGender>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().withData, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().optional, MakeArray<>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().optionalWithData, MakeArray<ComponentEmployee, ComponentGender>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().subtractive, MakeArray<ComponentDesigner>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().writeAccess, MakeArray<ComponentArtist, ComponentSpouse, ComponentEmployee>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().all, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, TagTest0, TagTest1, ComponentDateOfBirth, ComponentEmployee, ComponentGender, ComponentDesigner>()));
		static_assert(matches(TestSystem::AcquireRequestedComponents().argumentPassingOrder, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender>(), false));
		static_assert(testArchetypeDefining(TestSystem::AcquireRequestedComponents().archetypeDefiningInfoOnly, MakeArray<ComponentArtist, ComponentProgrammer, ComponentSpouse, ComponentCompany, TagTest0, TagTest1, ComponentDateOfBirth, ComponentDesigner>(), array<TypeAndGroup, 0>{}));
		static_assert(TestSystem::AcquireRequestedComponents().idsArgumentIndex == 4);
		static_assert(TestSystem::AcquireRequestedComponents().environmentArgumentIndex == 3);

		static_assert(matches(TestSystem2::AcquireRequestedComponents().requiredWithoutData, MakeArray<TagTest0, TagTest1, TagTest2>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().requiredWithData, MakeArray<>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().required, MakeArray<TagTest0, TagTest1, TagTest2>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().requiredOrOptional, MakeArray<TagTest0, TagTest1, TagTest2>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().withData, MakeArray<>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().optional, MakeArray<>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().optionalWithData, MakeArray<>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().subtractive, MakeArray<TagTest3, TagTest4, TagTest5>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().writeAccess, MakeArray<>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().all, MakeArray<TagTest0, TagTest1, TagTest2, TagTest3, TagTest4, TagTest5>()));
		static_assert(matches(TestSystem2::AcquireRequestedComponents().argumentPassingOrder, MakeArray<>(), false));
		static_assert(testArchetypeDefining(TestSystem2::AcquireRequestedComponents().archetypeDefiningInfoOnly, MakeArray<TagTest0, TagTest1, TagTest2, TagTest3, TagTest4, TagTest5>(), array<TypeAndGroup, 0>{}));
		static_assert(TestSystem2::AcquireRequestedComponents().idsArgumentIndex == nullopt);
		static_assert(TestSystem2::AcquireRequestedComponents().environmentArgumentIndex == nullopt);

		static_assert(matches(TestSystem3::AcquireRequestedComponents().requiredWithoutData, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().requiredWithData, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().required, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().requiredOrOptional, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().withData, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().optional, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().optionalWithData, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().subtractive, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().writeAccess, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().all, MakeArray<>()));
		static_assert(matches(TestSystem3::AcquireRequestedComponents().argumentPassingOrder, MakeArray<>(), false));
		static_assert(testArchetypeDefining(TestSystem3::AcquireRequestedComponents().archetypeDefiningInfoOnly, MakeArray<>(), array<TypeAndGroup, 0>{}));
		static_assert(TestSystem3::AcquireRequestedComponents().idsArgumentIndex == nullopt);
		static_assert(TestSystem3::AcquireRequestedComponents().environmentArgumentIndex == nullopt);

		auto test = TestSystem4::AcquireRequestedComponents().archetypeDefiningInfoOnly;
		int a = 0;

		static_assert(matches(TestSystem4::AcquireRequestedComponents().requiredWithoutData, MakeArray<>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().requiredWithData, MakeArray<ComponentSpouse, ComponentCompany>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().required, MakeArray<ComponentSpouse, ComponentCompany>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().requiredOrOptional, MakeArray<ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender, ComponentDateOfBirth, ComponentDesigner>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().withData, MakeArray<ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().optional, MakeArray<ComponentDateOfBirth, ComponentDesigner>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().optionalWithData, MakeArray<ComponentEmployee, ComponentGender>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().subtractive, MakeArray<>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().writeAccess, MakeArray<ComponentSpouse, ComponentEmployee>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().all, MakeArray<ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender, ComponentDateOfBirth, ComponentDesigner>()));
		static_assert(matches(TestSystem4::AcquireRequestedComponents().argumentPassingOrder, MakeArray<ComponentSpouse, ComponentCompany, ComponentEmployee, ComponentGender>(), false));
		static_assert(testArchetypeDefining(TestSystem4::AcquireRequestedComponents().archetypeDefiningInfoOnly, MakeArray<ComponentSpouse, ComponentCompany>(), make_array(TypeAndGroup{ComponentEmployee::GetTypeId(), 0}, TypeAndGroup{ComponentGender::GetTypeId(), 0})));
		static_assert(TestSystem4::AcquireRequestedComponents().idsArgumentIndex == nullopt);
		static_assert(TestSystem4::AcquireRequestedComponents().environmentArgumentIndex == nullopt);
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
			uiw len = (uiw)rand() % 10 + 5;
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
				StableTypeId types[] =
				{
					ComponentFirstName::GetTypeId(),
					ComponentArtist::GetTypeId()
				};
				Archetype arch = Archetype::Create<StableTypeId>(ToArray(types));
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
					uiw alignment = (uiw)1 << Funcs::IndexOfLeastSignificantNonZeroBit((uiw)c.data);
					ASSUME(alignment >= alignof(ComponentFirstName));

					ComponentFirstName *casted = (ComponentFirstName *)c.data;
					ASSUME(!MemOps::Compare(casted->name.data(), refName.name.data(), refName.name.size()));
				};

				checkFirstName(c0.type == ComponentFirstName::GetTypeId() ? c0 : c1);

				auto checkArtist = [](const SerializedComponent &c)
				{
					uiw alignment = (uiw)1 << Funcs::IndexOfLeastSignificantNonZeroBit((uiw)c.data);
					ASSUME(alignment >= alignof(ComponentArtist));

					ComponentArtist *casted = (ComponentArtist *)c.data;
					ASSUME(casted->area == ComponentArtist::Areas::Concept);
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
			printf("finished message builder tests\n");
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