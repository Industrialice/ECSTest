#pragma once

#include <System.hpp>

namespace ECSTest
{
    INDIRECT_SYSTEM(SystemGameInfo)
    {
        DIRECT_ACCEPT_COMPONENTS(const TransformComponent &transform, const PhysicsComponent &physics, const BoxColliderComponent *boxCollider, const CapsuleColliderComponent *capsuleCollider, const MeshColliderComponent *meshCollider, const SphereColliderComponent *sphereCollider);
    };
}