#pragma once

#include <Component.hpp>
#include <EntityID.hpp>

namespace ECSTest
{
	COMPONENT(ComponentParents)
	{
		EntityID father;
		EntityID mother;
	};
}