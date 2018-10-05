#include "PreHeader.hpp"
#include "JiggleBonesSystem.hpp"

using namespace ECSTest;

auto JiggleBonesSystem::RequestedComponentsAll() const -> pair<const RequestedComponent *, uiw>
{
    return make_pair(_requiredComponents, CountOf(_requiredComponents));
}