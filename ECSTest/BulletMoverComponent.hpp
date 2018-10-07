#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct BulletMoverComponent final : public _BaseComponent<BulletMoverComponent>
    {
    };

    GENERATE_TYPE_ID_TO_TYPE(BulletMoverComponent);
}