#include "PreHeader.hpp"
#include "BulletMoverSystem.hpp"

using namespace ECSTest;

auto BulletMoverSystem::RequiredComponents() const -> pair<const RequiredComponent *, uiw>
{
    return make_pair(_requiredComponents, CountOf(_requiredComponents));
}
