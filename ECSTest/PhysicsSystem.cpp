#include "PreHeader.hpp"
#include "PhysicsSystem.hpp"

using namespace ECSTest;

void PhysicsSystem::Accept(const TransformComponent &transform, const PhysicsComponent &physics) const
{
}

bool PhysicsSystem::IsFatSystem() const
{
	return true;
}
