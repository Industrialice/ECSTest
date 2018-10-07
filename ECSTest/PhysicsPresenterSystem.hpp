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
    class PhysicsPresenterSystem final : public _SystemTypeIdentifiable<PhysicsPresenterSystem>
    {
        ACCEPT_COMPONENTS(TransformComponent &transform, PhysicsComponent &physics, const BoxColliderComponent *boxCollider, const CapsuleColliderComponent *capsuleCollider, const MeshColliderComponent *meshCollider, const SphereColliderComponent *sphereCollider);
        virtual bool IsFatSystem() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(PhysicsPresenterSystem);
}