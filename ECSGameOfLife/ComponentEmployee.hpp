#pragma once

#include <Component.hpp>
#include <EntityID.hpp>

namespace ECSTest
{
    struct ComponentEmployee : Component<ComponentEmployee>
    {
        EntityID employer;
    };
}