#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"
#include "BoxColliderComponent.hpp"
#include "CapsuleColliderComponent.hpp"
#include "MeshColliderComponent.hpp"
#include "SphereColliderComponent.hpp"

namespace ECSTest
{
    class PhysicsSystem final : public _SystemTypeIdentifiable<PhysicsSystem>
    {
        ACCEPT_COMPONENTS(const TransformComponent &transform, const PhysicsComponent &physics, const BoxColliderComponent *boxCollider, const CapsuleColliderComponent *capsuleCollider, const MeshColliderComponent *meshCollider, const SphereColliderComponent *sphereCollider);
		[[nodiscard]] virtual bool IsFatSystem() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(PhysicsSystem);
}