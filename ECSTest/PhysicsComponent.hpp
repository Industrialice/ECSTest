#pragma once

#include "Component.hpp"

namespace ECSTest
{
    class PhysicsComponent final : public _BaseComponent<PhysicsComponent>
    {};

    GENERATE_TYPE_ID_TO_TYPE(PhysicsComponent);
}