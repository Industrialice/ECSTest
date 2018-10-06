#include "PreHeader.hpp"
#include "PhysicsSystem.hpp"

using namespace ECSTest;

auto PhysicsSystem::RequestedComponents() const -> pair<const RequestedComponent *, uiw>
{
    return make_pair(_requestedComponents, CountOf(_requestedComponents));
}

bool PhysicsSystem::IsFatSystem() const
{
	return true;
}
