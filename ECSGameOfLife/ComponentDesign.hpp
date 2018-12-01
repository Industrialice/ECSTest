#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentDesign : public _BaseComponent<ComponentDesign>
    {
        ENUM_COMBINABLE(Area, ui32,
            Level, UXUI);
        Area area;
    };
}