#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"
#include "JiggleBonesComponent.hpp"

namespace ECSTest
{
    class JiggleBonesSystem final : public _SystemTypeIdentifiable<JiggleBonesSystem>
    {
        static constexpr RequestedComponent _requiredComponents[] =
        {
            {TransformComponent::GetTypeId(), true, false},
            {PhysicsComponent::GetTypeId(), true, true},
            {JiggleBonesComponent::GetTypeId(), true, true}
        };

    public:
        virtual pair<const RequestedComponent *, uiw> RequestedComponentsAll() const override;
    };
}