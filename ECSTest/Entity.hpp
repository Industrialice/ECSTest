#pragma once

#include "Component.hpp"

namespace ECSTest
{
    class Entity
    {
	protected:
        void *_systemDataPointrs[MaxSystemDataPointers]{};
        std::list<void *> _extraSystemDataPointers{};
        Entity *_parent = nullptr;
        vector<unique_ptr<Component>> _components{};
		string _name{};
		bool _isEnabled = true;

		void AddComponent(unique_ptr<Component> component);
		void RemoveComponent(const Component &component);

    public:
		[[nodiscard]] const vector<unique_ptr<const Component>> &Components() const;
		[[nodiscard]] const string &Name() const;
		[[nodiscard]] const Entity *Parent() const;
    };
}