#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentDateOfBirth : public _BaseComponent<ComponentDateOfBirth>
    {
        ui32 dateOfBirth; // days since 01/01/2000
    };
}