#include "PreHeader.hpp"
#include "MeshRendererComponent.hpp"
#include "SkinnedMeshRendererComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> MeshRendererComponent::Excludes() const
{
    static constexpr TypeId excludes[] =
    {
        MeshRendererComponent::GetTypeId(),
        SkinnedMeshRendererComponent::GetTypeId()
    };
    return {excludes, CountOf(excludes)};
}
