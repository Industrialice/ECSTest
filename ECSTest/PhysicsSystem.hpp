#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"

namespace ECSTest
{
    class PhysicsSystem final : public _SystemTypeIdentifiable<PhysicsSystem>
    {
        ACCEPT_COMPONENTS(const TransformComponent &transform, const PhysicsComponent &physics);
		virtual bool IsFatSystem() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(PhysicsSystem);
}