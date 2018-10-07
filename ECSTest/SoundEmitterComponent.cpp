#include "PreHeader.hpp"
#include "SoundEmitterComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> SoundEmitterComponent::Excludes() const
{
    return {nullptr, 0};
}
