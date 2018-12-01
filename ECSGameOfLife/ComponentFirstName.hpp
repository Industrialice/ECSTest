#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentFirstName : public _BaseComponent<ComponentFirstName>
    {
        array<char, 32> name;
    };
}