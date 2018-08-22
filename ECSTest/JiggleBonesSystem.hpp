#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"
#include "JiggleBonesComponent.hpp"

namespace ECSTest
{
    class JiggleBonesSystem final : public System
    {
        static constexpr RequiredComponent _requiredComponents[] =
        {
            {TransformComponent::GetTypeId(), true, false},
            {PhysicsComponent::GetTypeId(), true, true},
            {JiggleBonesComponent::GetTypeId(), true, true}
        };

    public:
        virtual pair<const RequiredComponent *, uiw> RequiredComponents() const override;
    };
}