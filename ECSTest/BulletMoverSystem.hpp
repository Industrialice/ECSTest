#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "BulletMoverComponent.hpp"

namespace ECSTest
{
    class BulletMoverSystem final : public _DirectSystem<BulletMoverSystem>
    {
        DIRECT_ACCEPT_COMPONENTS(Array<TransformComponent> &transform, const Array<BulletMoverComponent> &bulletMover);
    };

    GENERATE_TYPE_ID_TO_TYPE(BulletMoverSystem);
}