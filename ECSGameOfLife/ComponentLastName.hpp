#pragma once

#include <Component.hpp>

namespace ECSTest
{
	struct ComponentLastName : Component<ComponentLastName>
    {
        array<char, 32> name;
    };
}