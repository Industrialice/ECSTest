#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct SkinnedMeshRendererComponent final : public _BaseComponent<SkinnedMeshRendererComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(SkinnedMeshRendererComponent);
}