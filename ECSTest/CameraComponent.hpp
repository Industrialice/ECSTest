#pragma once

#include "Component.hpp"

namespace ECSTest
{
	struct CameraComponent final : public _BaseComponent<CameraComponent>
	{
	};

    GENERATE_TYPE_ID_TO_TYPE(CameraComponent);
}