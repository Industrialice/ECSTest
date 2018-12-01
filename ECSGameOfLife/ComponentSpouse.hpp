#pragma once

#include <Component.hpp>
#include <Entity.hpp>

namespace ECSTest
{
    struct ComponentSpouse : public _BaseComponent<ComponentSpouse>
    {
        EntityID spouse;
        ui32 dateOfMarriage; // days since 01/01/2000
    };
}