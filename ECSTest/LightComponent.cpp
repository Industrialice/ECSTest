#include "PreHeader.hpp"
#include "LightComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> LightComponent::Excludes() const
{
    return {nullptr, 0};
}
