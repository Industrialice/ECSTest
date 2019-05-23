#pragma once

#include <Component.hpp>

namespace ECSTest
{
	struct ComponentFirstName : Component<ComponentFirstName>
    {
        array<char, 32> name;
    };
}