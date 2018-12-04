#pragma once

#include <Component.hpp>
#include <EntityID.hpp>

namespace ECSTest
{
    struct ComponentEmployee : public _BaseComponent<ComponentEmployee>
    {
        EntityID employer;
    };
}