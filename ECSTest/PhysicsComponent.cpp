#include "PreHeader.hpp"
#include "PhysicsComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> PhysicsComponent::Excludes() const
{
    static constexpr TypeId excludes[] = {PhysicsComponent::GetTypeId()};
    return {excludes, CountOf(excludes)};
}
