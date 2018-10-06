#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "PhysicsComponent.hpp"

namespace ECSTest
{
    class PhysicsSystem final : public _SystemTypeIdentifiable<PhysicsSystem>
    {
        static constexpr RequestedComponent _requestedComponents[] =
        {
            {TransformComponent::GetTypeId(), true, true, System::ComponentOptionality::Required},
            {PhysicsComponent::GetTypeId(), true, true, System::ComponentOptionality::Required}
        };

    public:
        virtual pair<const RequestedComponent *, uiw> RequestedComponents() const override;
		virtual bool IsFatSystem() const;
    };

    GENERATE_TYPE_ID_TO_TYPE(PhysicsSystem);
}