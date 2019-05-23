#pragma once

#include <Component.hpp>

namespace ECSTest
{
	struct ComponentGender : Component<ComponentGender>
    {
        bool isMale;
    };
}