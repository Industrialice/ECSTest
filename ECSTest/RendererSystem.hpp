#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "CameraComponent.hpp"
#include "LightComponent.hpp"

namespace ECSTest
{
	class RendererSystem final : public _SystemTypeIdentifiable<RendererSystem>
	{
        ACCEPT_COMPONENTS(const TransformComponent &transform, const MeshRendererComponent *meshRenderer, const CameraComponent *camera, const LightComponent *light);
		virtual bool IsFatSystem() const override;
	};

    GENERATE_TYPE_ID_TO_TYPE(RendererSystem);
}