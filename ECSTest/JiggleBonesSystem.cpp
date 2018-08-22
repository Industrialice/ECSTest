#include "PreHeader.hpp"
#include "JiggleBonesSystem.hpp"

using namespace ECSTest;

auto JiggleBonesSystem::RequiredComponents() const -> pair<const RequiredComponent *, uiw>
{
    return make_pair(_requiredComponents, CountOf(_requiredComponents));
}
