#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "BulletMoverComponent.hpp"

namespace ECSTest
{
    class BulletMoverSystem final : public System
    {
        static constexpr RequiredComponent _requiredComponents[] =
        {
            {TransformComponent::GetTypeId(), true, true},
            {BulletMoverComponent::GetTypeId(), true, false}
        };

    public:
        virtual pair<const RequiredComponent *, uiw> RequiredComponents() const override;
    };
}