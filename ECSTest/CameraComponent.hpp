#pragma once

#include "Component.hpp"

namespace ECSTest
{
	struct CameraComponent final : public _BaseComponent<CameraComponent>
	{
        virtual pair<const TypeId *, uiw> Excludes() const override;
    };

    GENERATE_TYPE_ID_TO_TYPE(CameraComponent);
}