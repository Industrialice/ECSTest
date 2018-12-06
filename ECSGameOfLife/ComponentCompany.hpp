#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentCompany)
    {
        array<char, 32> name;
    };
}