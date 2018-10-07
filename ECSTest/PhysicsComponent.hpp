#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct PhysicsComponent final : public _BaseComponent<PhysicsComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(PhysicsComponent);
}