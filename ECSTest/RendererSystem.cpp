#include "PreHeader.hpp"
#include "RendererSystem.hpp"

using namespace ECSTest;

void RendererSystem::Accept(const TransformComponent &transform, const MeshRendererComponent *meshRenderer, const CameraComponent *camera, const LightComponent *light) const
{
}

bool RendererSystem::IsFatSystem() const
{
	return true;
}
