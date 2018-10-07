#include "PreHeader.hpp"
#include "CameraComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> CameraComponent::Excludes() const
{
    static constexpr TypeId excludes[] = {CameraComponent::GetTypeId()};
    return {excludes, CountOf(excludes)};
}
