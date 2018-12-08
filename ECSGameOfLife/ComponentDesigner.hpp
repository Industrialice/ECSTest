#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentDesigner)
    {
        struct Areas
        {
            static constexpr struct Area : EnumCombinable<Area, ui32, true>
            {} Undefined = Area::Create(0),
                Level = Area::Create(1 << 0),
                UXUI = Area::Create(1 << 1);
        };

        Areas::Area area;
    };
}