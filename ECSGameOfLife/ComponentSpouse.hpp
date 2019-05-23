#pragma once

#include <Component.hpp>
#include <EntityID.hpp>

namespace ECSTest
{
	struct ComponentSpouse : Component<ComponentSpouse>
    {
        EntityID spouse;
        ui32 dateOfMarriage; // days since 01/01/2000
    };
}