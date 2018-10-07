#pragma once

#include "Component.hpp"

namespace ECSTest
{
	struct MeshRendererComponent final : public _BaseComponent<MeshRendererComponent>
	{
        virtual pair<const TypeId *, uiw> Excludes() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(MeshRendererComponent);
}