#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentDesigner)
    {
		ENUM_COMBINABLE(Area, ui32,
            Undefined,
			Level = Funcs::BitPos(0),
			UXUI = Funcs::BitPos(1));

        Area area;
    };

	ENUM_COMBINABLE_OPS(ComponentDesigner::Area, ui32);
}