#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"
#include "JiggleBonesComponent.hpp"

namespace ECSTest
{
    class JiggleBonesSystem final : public _SystemTypeIdentifiable<JiggleBonesSystem>
    {
        ACCEPT_SLIM_COMPONENTS(const TransformComponent &transform, PhysicsComponent &physics, JiggleBonesComponent &jiggleBones);
    };

    GENERATE_TYPE_ID_TO_TYPE(JiggleBonesSystem);
}