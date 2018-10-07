#include "PreHeader.hpp"
#include "PhysicsPresenterSystem.hpp"

using namespace ECSTest;

void PhysicsPresenterSystem::Accept(TransformComponent &transform, PhysicsComponent &physics, const BoxColliderComponent *boxCollider, const CapsuleColliderComponent *capsuleCollider, const MeshColliderComponent *meshCollider, const SphereColliderComponent *sphereCollider) const
{}

bool PhysicsPresenterSystem::IsFatSystem() const
{
    return true;
}
