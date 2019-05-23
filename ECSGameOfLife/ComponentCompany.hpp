#pragma once

#include <Component.hpp>

namespace ECSTest
{
	struct ComponentCompany : Component<ComponentCompany>
    {
        array<char, 32> name;
    };
}