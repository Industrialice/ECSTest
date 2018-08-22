#pragma once

#include "Entity.hpp"

namespace ECSTest
{
    class Scene
    {
        vector<unique_ptr<Entity>> _entities{};

    public:
        void AddEntity(unique_ptr<Entity> entity);
    };
}