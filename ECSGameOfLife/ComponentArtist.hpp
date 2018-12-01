#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentArtist : public _BaseComponent<ComponentArtist>
    {
        ENUM_COMBINABLE(Area, ui32,
            TwoD, ThreeD, Concept);
        Area area;
    };
}