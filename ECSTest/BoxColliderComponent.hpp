#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct BoxColliderComponent final : public _BaseComponent<BoxColliderComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(BoxColliderComponent);
}