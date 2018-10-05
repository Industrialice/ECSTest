#include "PreHeader.hpp"
#include "RendererSystem.hpp"

using namespace ECSTest;

auto RendererSystem::RequestedComponentsAll() const -> pair<const RequestedComponent *, uiw>
{
	return {_requiredComponentsAll, CountOf(_requiredComponentsAll)};
}

auto RendererSystem::RequestedComponentsAny() const -> pair<const RequestedComponent *, uiw>
{
	return {_requiredComponentsAny, CountOf(_requiredComponentsAny)};
}

bool RendererSystem::IsFatSystem() const
{
	return true;
}
