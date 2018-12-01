#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentCompany : public _BaseComponent<ComponentCompany>
    {
        array<char, 32> name;
    };
}