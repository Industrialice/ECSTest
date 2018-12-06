#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentDateOfBirth)
    {
        ui32 dateOfBirth; // days since 01/01/2000
    };
}