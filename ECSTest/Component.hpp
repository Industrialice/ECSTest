#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Component
    {
    };

    template <typename T> class _BaseComponent : public Component, public TypeIdentifiable<T>
    {};
}