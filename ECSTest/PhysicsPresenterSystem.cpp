#include "PreHeader.hpp"
#include "PhysicsPresenterSystem.hpp"

using namespace ECSTest;

void PhysicsPresenterSystem::Accept(TransformComponent &transform, PhysicsComponent &physics) const
{}

bool PhysicsPresenterSystem::IsFatSystem() const
{
    return true;
}
