#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentDesign : public _BaseComponent<ComponentDesign>
    {
		ENUM_COMBINABLE(Area, ui32,
			Level = Funcs::BitPos(0),
			UXUI = Funcs::BitPos(1));

        Area area;
    };

	ENUM_COMBINABLE_OPS(ComponentDesign::Area, ui32);
}