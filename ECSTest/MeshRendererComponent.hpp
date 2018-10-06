#pragma once

#include "Component.hpp"

namespace ECSTest
{
	struct MeshRendererComponent final : public _BaseComponent<MeshRendererComponent>
	{
	};

    GENERATE_TYPE_ID_TO_TYPE(MeshRendererComponent);
}