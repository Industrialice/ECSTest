#pragma once

#include <Component.hpp>

namespace ECSTest
{
	struct ComponentArtist : NonUniqueComponent<ComponentArtist>
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