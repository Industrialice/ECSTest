#pragma once

#include <Component.hpp>

namespace ECSTest
{
	struct ComponentDateOfBirth : Component<ComponentDateOfBirth>
    {
        ui32 dateOfBirth; // days since 01/01/2000
    };
}