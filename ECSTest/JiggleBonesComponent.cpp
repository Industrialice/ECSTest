#include "PreHeader.hpp"
#include "JiggleBonesComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> JiggleBonesComponent::Excludes() const
{
    static constexpr TypeId excludes[] = {JiggleBonesComponent::GetTypeId()};
    return {excludes, CountOf(excludes)};
}
