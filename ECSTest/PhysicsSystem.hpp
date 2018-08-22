#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"

namespace ECSTest
{
    class PhysicsSystem final : public System
    {
        static constexpr RequiredComponent _requiredComponents[] =
        {
            {TransformComponent::GetTypeId(), true, true},
            {PhysicsComponent::GetTypeId(), true, true}
        };

    public:
        virtual pair<const RequiredComponent *, uiw> RequiredComponents() const override;
    };
}