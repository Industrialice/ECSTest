#pragma once

#include <Component.hpp>
#include <EntityID.hpp>

namespace ECSTest
{
	struct ComponentParents : Component<ComponentParents>
	{
		EntityID father;
		EntityID mother;
	};
}