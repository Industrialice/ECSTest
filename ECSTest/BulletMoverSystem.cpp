#include "PreHeader.hpp"
#include "BulletMoverSystem.hpp"

using namespace ECSTest;

auto BulletMoverSystem::RequestedComponentsAll() const -> pair<const RequestedComponent *, uiw>
{
	return {_requiredComponents, CountOf(_requiredComponents)};
}
