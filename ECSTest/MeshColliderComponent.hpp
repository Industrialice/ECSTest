#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct MeshColliderComponent final : public _BaseComponent<MeshColliderComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(MeshColliderComponent);
}