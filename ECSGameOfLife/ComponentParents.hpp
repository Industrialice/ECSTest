#pragma once

#include <Component.hpp>
#include <EntityID.hpp>

namespace ECSTest
{
	struct ComponentParents : public _BaseComponent<ComponentParents>
	{
		EntityID father;
		EntityID mother;
	};
}