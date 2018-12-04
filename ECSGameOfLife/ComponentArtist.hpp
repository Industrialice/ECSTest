#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentArtist : public _BaseComponent<ComponentArtist>
    {
		ENUM_COMBINABLE(Area, ui32,
			TwoD = Funcs::BitPos(0),
			ThreeD = Funcs::BitPos(1),
			Concept = Funcs::BitPos(2));

        Area area;
    };

	ENUM_COMBINABLE_OPS(ComponentArtist::Area, ui32);
}