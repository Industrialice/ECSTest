#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentFirstName)
    {
        array<char, 32> name;
    };
}