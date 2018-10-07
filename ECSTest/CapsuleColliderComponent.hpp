#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct CapsuleColliderComponent final : public _BaseComponent<CapsuleColliderComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(CapsuleColliderComponent);
}