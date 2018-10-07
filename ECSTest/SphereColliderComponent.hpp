#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct SphereColliderComponent final : public _BaseComponent<SphereColliderComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(SphereColliderComponent);
}