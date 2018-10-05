#pragma once

#include "Component.hpp"

namespace ECSTest
{
    class Entity
    {
        void *_systemDataPointrs[MaxSystemDataPointers]{};
        std::list<void *> _extraSystemDataPointers{};
        Entity *_parent = nullptr;
        vector<unique_ptr<Component>> _components{};

    public:
        void AddComponent(unique_ptr<Component> component);
		void RemoveComponent(const Component &component);
    };
}