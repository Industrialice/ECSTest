#include "PreHeader.hpp"
#include "BoxColliderComponent.hpp"
#include "CapsuleColliderComponent.hpp"
#include "MeshColliderComponent.hpp"
#include "SphereColliderComponent.hpp"

using namespace ECSTest;

pair<const TypeId *, uiw> MeshColliderComponent::Excludes() const
{
    static constexpr TypeId excludes[] =
    {
        BoxColliderComponent::GetTypeId(),
        CapsuleColliderComponent::GetTypeId(),
        MeshColliderComponent::GetTypeId(),
        SphereColliderComponent::GetTypeId()
    };
    return {excludes, CountOf(excludes)};
}
