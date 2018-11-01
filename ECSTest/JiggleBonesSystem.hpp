#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"
#include "JiggleBonesComponent.hpp"

namespace ECSTest
{
    class JiggleBonesSystem final : public _DirectSystem<JiggleBonesSystem>
    {
        ACCEPT_COMPONENTS(const Array<TransformComponent> &transform, Array<PhysicsComponent> &physics, Array<JiggleBonesComponent> &jiggleBones);
    };

    GENERATE_TYPE_ID_TO_TYPE(JiggleBonesSystem);
}