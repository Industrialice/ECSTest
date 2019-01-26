#include "PreHeader.hpp"
#include <ArchetypeReflector.hpp>
#include <System.hpp>

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

template <uiw size> static Archetype GenerateArchetype(const array<StableTypeId, size> &source)
{
    Archetype archetype;
    for (auto &req : source)
    {
        archetype.Add(req);
    }
    return archetype;
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
        result.push_back({req.type, req.requirement});
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
    auto arch0 = GenerateArchetype(ent0);

    array<StableTypeId, 3> ent1 =
    {
        ComponentCompany::GetTypeId(),
        ComponentProgrammer::GetTypeId(),
        ComponentArtist::GetTypeId()
    };
    auto arch1 = GenerateArchetype(ent1);

    // same as ent1, but different order
    array<StableTypeId, 3> ent2 =
    {
        ComponentProgrammer::GetTypeId(),
        ComponentCompany::GetTypeId(),
        ComponentArtist::GetTypeId()
    };
    auto arch2 = GenerateArchetype(ent2);
    ASSUME(arch1 == arch2);

    array<StableTypeId, 5> ent3 =
    {
        ComponentSpouse::GetTypeId(),
        ComponentLastName::GetTypeId(),
        ComponentFirstName::GetTypeId(),
        ComponentEmployee::GetTypeId(),
        ComponentDesigner::GetTypeId()
    };
    auto arch3 = GenerateArchetype(ent3);

    array<StableTypeId, 4> ent4 =
    {
        ComponentProgrammer::GetTypeId(),
        ComponentDesigner::GetTypeId(),
        ComponentSpouse::GetTypeId(),
        ComponentGender::GetTypeId()
    };
    auto arch4 = GenerateArchetype(ent4);

    array<StableTypeId, 5> ent5 =
    {
        ComponentProgrammer::GetTypeId(),
        ComponentDesigner::GetTypeId(),
        ComponentSpouse::GetTypeId(),
        ComponentGender::GetTypeId(),
        ComponentCompany::GetTypeId()
    };
    auto arch5 = GenerateArchetype(ent5);

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
    reflector.AddToLibrary(arch0, ToTypes(ent0));
    reflector.AddToLibrary(arch1, ToTypes(ent1));

    auto reflected = reflector.Reflect(arch0);
    ASSUME(IsReflectedEqual(reflected, ent0));

    reflected = reflector.Reflect(arch1);
    ASSUME(IsReflectedEqual(reflected, ent1));

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

    reflector.AddToLibrary(arch2, ToTypes(ent2));
    reflector.AddToLibrary(arch3, ToTypes(ent3));
    reflector.AddToLibrary(arch4, ToTypes(ent4));
    reflector.AddToLibrary(arch5, ToTypes(ent5));

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
}

int main()
{
    Initialization::Initialize({});

    ReflectorTests();

    system("pause");
}