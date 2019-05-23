#pragma once

#include <Component.hpp>

namespace ECSTest
{
	struct ComponentDesigner : NonUniqueComponent<ComponentDesigner>
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