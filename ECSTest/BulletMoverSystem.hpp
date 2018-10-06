#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "BulletMoverComponent.hpp"

namespace ECSTest
{
    class BulletMoverSystem final : public _SystemTypeIdentifiable<BulletMoverSystem>
    {
        ACCEPT_SLIM_COMPONENTS(TransformComponent &transform, const BulletMoverComponent &bulletMover);
    };

    GENERATE_TYPE_ID_TO_TYPE(BulletMoverSystem);
}