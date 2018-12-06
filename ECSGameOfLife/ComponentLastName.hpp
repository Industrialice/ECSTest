#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentLastName)
    {
        array<char, 32> name;
    };
}