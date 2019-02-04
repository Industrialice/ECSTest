#pragma once

#include <Component.hpp>

namespace ECSTest
{
	NONUNIQUE_COMPONENT(ComponentDesigner)
    {
        enum class Areas : ui8
        {
            Undefined,
            Level,
            UXUI
        };

        Areas area;
    };
}