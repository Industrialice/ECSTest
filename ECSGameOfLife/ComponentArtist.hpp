#pragma once

#include <Component.hpp>

namespace ECSTest
{
	NONUNIQUE_COMPONENT(ComponentArtist)
    {
        enum class Areas : ui8
        {
            Undefined,
            TwoD,
            ThreeD,
            Concept
        };

        Areas area;
    };
}