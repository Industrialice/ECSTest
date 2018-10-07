#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct PhysicsComponent final : public _BaseComponent<PhysicsComponent>
    {
    };

    GENERATE_TYPE_ID_TO_TYPE(PhysicsComponent);
}