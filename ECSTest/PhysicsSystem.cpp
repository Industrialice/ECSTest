#include "PreHeader.hpp"
#include "PhysicsSystem.hpp"

using namespace ECSTest;

auto PhysicsSystem::RequiredComponents() const -> pair<const RequiredComponent *, uiw>
{
    return make_pair(_requiredComponents, CountOf(_requiredComponents));
}
