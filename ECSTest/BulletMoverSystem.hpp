#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "BulletMoverComponent.hpp"

namespace ECSTest
{
    class BulletMoverSystem final : public _SystemTypeIdentifiable<BulletMoverSystem>
    {
        static constexpr RequestedComponent _requiredComponents[] =
        {
            {TransformComponent::GetTypeId(), true, true},
            {BulletMoverComponent::GetTypeId(), true, false}
        };

    public:
        virtual pair<const RequestedComponent *, uiw> RequestedComponentsAll() const override;
    };
}