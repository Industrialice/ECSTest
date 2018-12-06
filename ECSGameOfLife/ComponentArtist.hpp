#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentArtist)
    {
		ENUM_COMBINABLE(Area, ui32,
            Undefined,
			TwoD = Funcs::BitPos(0),
			ThreeD = Funcs::BitPos(1),
			Concept = Funcs::BitPos(2));

        Area area;
    };

	ENUM_COMBINABLE_OPS(ComponentArtist::Area, ui32);
}