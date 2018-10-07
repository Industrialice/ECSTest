#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Component
    {
    public:
        class Entity &Entity();
        const class Entity &Entity() const;
    };

    template <typename T> class _BaseComponent : public Component, public TypeIdentifiable<T>
    {};
}