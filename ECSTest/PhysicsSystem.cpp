#include "PreHeader.hpp"
#include "PhysicsSystem.hpp"

using namespace ECSTest;

auto PhysicsSystem::RequestedComponentsAll() const -> pair<const RequestedComponent *, uiw>
{
    return make_pair(_requiredComponents, CountOf(_requiredComponents));
}

bool PhysicsSystem::IsFatSystem() const
{
	return true;
}
