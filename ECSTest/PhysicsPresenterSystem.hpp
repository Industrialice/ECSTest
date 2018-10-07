#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"

namespace ECSTest
{
    class PhysicsPresenterSystem final : public _SystemTypeIdentifiable<PhysicsPresenterSystem>
    {
        ACCEPT_COMPONENTS(TransformComponent &transform, PhysicsComponent &physics);
        virtual bool IsFatSystem() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(PhysicsPresenterSystem);
}