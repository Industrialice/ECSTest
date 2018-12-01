#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentGender : public _BaseComponent<ComponentGender>
    {
        bool isMale;
    };
}