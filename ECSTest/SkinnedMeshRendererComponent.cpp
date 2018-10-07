#include "PreHeader.hpp"
#include "SkinnedMeshRendererComponent.hpp"
#include "MeshRendererComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> SkinnedMeshRendererComponent::Excludes() const
{
    static constexpr TypeId excludes[] =
    {
        MeshRendererComponent::GetTypeId(),
        SkinnedMeshRendererComponent::GetTypeId()
    };
    return {excludes, CountOf(excludes)};
}
