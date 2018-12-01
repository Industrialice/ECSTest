#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentLastName : public _BaseComponent<ComponentLastName>
    {
        array<char, 32> name;
    };
}