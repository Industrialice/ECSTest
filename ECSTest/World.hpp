#pragma once

#include "Entity.hpp"

namespace ECSTest
{
    class World
    {
        vector<unique_ptr<Entity>> _entities{};

    public:
        void AddEntity(unique_ptr<Entity> entity);
    };
}