#include "PreHeader.hpp"
#include "BulletMoverComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> BulletMoverComponent::Excludes() const
{
    static constexpr TypeId excludes[] = {BulletMoverComponent::GetTypeId()};
    return {excludes, CountOf(excludes)};
}
