#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "CameraComponent.hpp"

namespace ECSTest
{
	class RendererSystem final : public _SystemTypeIdentifiable<RendererSystem>
	{
		static constexpr RequestedComponent _requiredComponents[] =
        {
            {TransformComponent::GetTypeId(), true, false, System::ComponentOptionality::Required},
            {MeshRendererComponent::GetTypeId(), true, false, System::ComponentOptionality::Any},
            {CameraComponent::GetTypeId(), true, false, System::ComponentOptionality::Any}
        };

	public:
		virtual pair<const RequestedComponent *, uiw> RequestedComponents() const override;
		virtual bool IsFatSystem() const override;
	};

    GENERATE_TYPE_ID_TO_TYPE(RendererSystem);
}