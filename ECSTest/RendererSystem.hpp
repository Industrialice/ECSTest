#pragma once

#include "System.hpp"
#include "TransformComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "CameraComponent.hpp"

namespace ECSTest
{
	class RendererSystem final : public _SystemTypeIdentifiable<RendererSystem>
	{
		static constexpr RequestedComponent _requiredComponentsAll[] =
		{
			{TransformComponent::GetTypeId(), true, false}
		};

		static constexpr RequestedComponent _requiredComponentsAny[] =
		{
			{MeshRendererComponent::GetTypeId(), true, false},
			{CameraComponent::GetTypeId(), true, false}
		};

	public:
		virtual pair<const RequestedComponent *, uiw> RequestedComponentsAll() const override;
		virtual pair<const RequestedComponent *, uiw> RequestedComponentsAny() const override;
		virtual bool IsFatSystem() const override;
	};
}