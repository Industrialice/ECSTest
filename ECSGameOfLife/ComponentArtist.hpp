#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentArtist)
    {
        struct Areas
        {
            static constexpr struct Area : EnumCombinable<Area, ui32, true>
            {} Undefined = Area::Create(0),
                TwoD = Area::Create(1 << 0),
                ThreeD = Area::Create(1 << 1),
                Concept = Area::Create(1 << 2);
        };

        Areas::Area area;
    };
}