#include "PreHeader.hpp"
#include <ArchetypeReflector.hpp>
#include <System.hpp>
#include <MessageBuilder.hpp>

#include <ComponentArtist.hpp>
#include <ComponentCompany.hpp>
#include <ComponentDateOfBirth.hpp>
#include <ComponentDesigner.hpp>
#include <ComponentEmployee.hpp>
#include <ComponentFirstName.hpp>
#include <ComponentGender.hpp>
#include <ComponentLastName.hpp>
#include <ComponentParents.hpp>
#include <ComponentProgrammer.hpp>
#include <ComponentSpouse.hpp>

using namespace ECSTest;

static void ArchetypeTests()
{
    StableTypeId types0[] = {ComponentArtist::GetTypeId(), ComponentArtist::GetTypeId()};
    auto shor = Archetype::Create<StableTypeId>(ToArray(types0));

    StableTypeId types1[] = {ComponentArtist::GetTypeId()};
    auto shor2 = Archetype::Create<StableTypeId>(ToArray(types1));

    ASSUME(shor == shor2);

    using typeId = pair<StableTypeId, ComponentID>;
    typeId types2[] = {{ComponentArtist::GetTypeId(), 0}, {ComponentArtist::GetTypeId(), 1}};
    auto arc = ArchetypeFull::Create<typeId, &typeId::first, &typeId::second>(ToArray(types2));

    typeId types3[] = {{ComponentArtist::GetTypeId(), 0}, {ComponentArtist::GetTypeId(), 1}, {ComponentArtist::GetTypeId(), 2}};
    auto arc2 = ArchetypeFull::Create<typeId, &typeId::first, &typeId::second>(ToArray(types3));
    ASSUME(arc != arc2);

    shor = arc.ToShort();
    shor2 = arc2.ToShort();
    ASSUME(shor == shor2);

    shor = Archetype::Create<typeId, &typeId::first>(ToArray(types2));
    shor2 = Archetype::Create<typeId, &typeId::first>(ToArray(types3));
    ASSUME(shor == shor2);

    printf("finished archetype tests\n");
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

template <uiw size> static vector<pair<StableTypeId, RequirementForComponent>> ToRequired(const array<System::RequestedComponent, size> &source)
{
    vector<pair<StableTypeId, RequirementForComponent>> result;
    for (auto &req : source)
    {
        if (req.requirement != RequirementForComponent::Optional)
        {
            result.push_back({req.type, req.requirement});
        }
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

static void ReflectorTests()
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

    // ent0 fits
    array<System::RequestedComponent, 3> req0 =
    {
        System::RequestedComponent{ComponentArtist::GetTypeId(), false, RequirementForComponent::Required},
        System::RequestedComponent{ComponentDateOfBirth::GetTypeId(), false, RequirementForComponent::Optional},
        System::RequestedComponent{ComponentCompany::GetTypeId(), false, RequirementForComponent::Subtractive},
    };
    req0 = Funcs::SortCompileTime(req0);

    // no entities fit
    array<System::RequestedComponent, 3> req1 =
    {
        System::RequestedComponent{ComponentProgrammer::GetTypeId(), false, RequirementForComponent::Required},
        System::RequestedComponent{ComponentDateOfBirth::GetTypeId(), false, RequirementForComponent::Required},
        System::RequestedComponent{ComponentFirstName::GetTypeId(), false, RequirementForComponent::Required},
    };
    req1 = Funcs::SortCompileTime(req1);

    // ent3, ent4 fit
    array<System::RequestedComponent, 3> req2 =
    {
        System::RequestedComponent{ComponentCompany::GetTypeId(), false, RequirementForComponent::Subtractive},
        System::RequestedComponent{ComponentDesigner::GetTypeId(), false, RequirementForComponent::Required},
        System::RequestedComponent{ComponentSpouse::GetTypeId(), false, RequirementForComponent::Required},
    };
    req2 = Funcs::SortCompileTime(req2);

    ArchetypeReflector reflector;
    reflector.AddToLibrary(arch0, ToTypes(Funcs::SortCompileTime(ent0)));
    reflector.AddToLibrary(arch1, ToTypes(Funcs::SortCompileTime(ent1)));

    auto reflected = reflector.Reflect(arch0);
    ASSUME(IsReflectedEqual(reflected, Funcs::SortCompileTime(ent0)));

    reflected = reflector.Reflect(arch1);
    ASSUME(IsReflectedEqual(reflected, Funcs::SortCompileTime(ent1)));

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

    printf("finished archetype reflector tests\n");
}

class UnitTests
{
public:
    static void MessageBuilderTests()
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
            uiw len = rand() % 10 + 5;
            for (auto index = 0; index < len; ++index)
            {
                name.name[index] = 'a' + rand() % 24;
            }
            return name;
        };

        for (ui32 index = 0; index < 10000; ++index)
        {
            {
                auto &componentBuilder = builder.AddEntity(gen.Generate());

                ComponentFirstName name = generateName();
                entityNames[gen.LastGenerated()] = name;

                ComponentArtist artist;
                artist.area = ComponentArtist::Areas::Concept;

                componentBuilder.AddComponent(name).AddComponent(artist, {});
            }

            if (rand() % 10 == 0)
            {
                StableTypeId types[] =
                {
                    ComponentFirstName::GetTypeId(),
                    ComponentArtist::GetTypeId()
                };
                Archetype arch = Archetype::Create<StableTypeId>(ToArray(types));
                builder.RemoveEntity(arch, gen.LastGenerated());
                auto [it, result] = entitiesRemoved.insert(gen.LastGenerated());
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
            MessageStreamEntityAdded addedStream(streamSource.first, streamSource.second);

            for (const auto &entity : addedStream)
            {
                ASSUME(entity.components.size() == 2);

                ++checked;

                const auto &c0 = entity.components[0];
                const auto &c1 = entity.components[1];

                ComponentFirstName refName = entityNames[entity.entityID];

                auto checkFirstName = [refName](const SerializedComponent &c)
                {
                    uiw alignment = (uiw)1 << Funcs::IndexOfLeastSignificantNonZeroBit((uiw)c.data);
                    ASSUME(alignment >= alignof(ComponentFirstName));

                    ComponentFirstName *casted = (ComponentFirstName *)c.data;
                    ASSUME(!memcmp(casted->name.data(), refName.name.data(), refName.name.size()));
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
            MessageStreamEntityRemoved removedStream(streamSource.first, streamSource.second);

            for (const auto &id : removedStream)
            {
                ASSUME(entitiesRemoved.find(id) != entitiesRemoved.end());
                ++checked;
            }
        }
        ASSUME(checked == entitiesRemoved.size());

        checked = 0;
        for (const auto &streamSource : builder.ComponentChangedStreams()._data)
        {
            MessageStreamComponentChanged changed(streamSource.first, streamSource.second);

            for (const auto &component : changed)
            {
                ++checked;
                ComponentFirstName *casted = (ComponentFirstName *)component.component.data;
                ASSUME(!memcmp(entityAfterChangeNames[component.entityID].name.data(), casted->name.data(), casted->name.size()));
            }
        }
        ASSUME(checked == entityAfterChangeNames.size());

        printf("finished message builder tests\n");
    }
};

int main()
{
    Initialization::Initialize({});

    ArchetypeTests();
    ReflectorTests();
    UnitTests::MessageBuilderTests();

    system("pause");
}