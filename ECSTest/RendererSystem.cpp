#include "PreHeader.hpp"
#include "RendererSystem.hpp"

using namespace ECSTest;

auto RendererSystem::RequestedComponents() const -> pair<const RequestedComponent *, uiw>
{
	return {_requiredComponents, CountOf(_requiredComponents)};
}

bool RendererSystem::IsFatSystem() const
{
	return true;
}
