#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"

namespace ECSTest
{
    class PhysicsSystem final : public _SystemTypeIdentifiable<PhysicsSystem>
    {
        static constexpr RequestedComponent _requiredComponents[] =
        {
            {TransformComponent::GetTypeId(), true, true},
            {PhysicsComponent::GetTypeId(), true, true}
        };

    public:
        virtual pair<const RequestedComponent *, uiw> RequestedComponentsAll() const override;
		virtual bool IsFatSystem() const;
    };
}