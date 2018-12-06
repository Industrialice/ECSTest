#pragma once

#include <Component.hpp>
#include <EntityID.hpp>

namespace ECSTest
{
    COMPONENT(ComponentEmployee)
    {
        EntityID employer;
    };
}