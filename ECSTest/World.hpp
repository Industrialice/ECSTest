#pragma once

#include "Entity.hpp"

namespace ECSTest
{
    class World
    {
        vector<unique_ptr<Entity>> _entities{};
		UniqueIdManager _idManager{};

    public:
        void AddEntity(unique_ptr<Entity> entity);
		UniqueIdManager &IDManager();
		const UniqueIdManager &IDManager() const;
    };
}