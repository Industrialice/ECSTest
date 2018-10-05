#include "PreHeader.hpp"
#include "System.hpp"

using namespace ECSTest;

auto System::RequestedComponentsAny() const -> pair<const RequestedComponent *, uiw>
{
	return {nullptr, 0};
}

auto System::RequestedComponentsOptional() const -> pair<const RequestedComponent *, uiw>
{
	return {nullptr, 0};
}

bool System::IsFatSystem() const
{
	return false;
}
