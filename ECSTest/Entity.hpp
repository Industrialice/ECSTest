#pragma once

#include "Component.hpp"

namespace ECSTest
{
	class EntityID
	{
		ui32 _id = ui32_max;

	public:
		EntityID(ui32 id);
		[[nodiscard]] bool operator == (const EntityID &other) const;
		[[nodiscard]] bool operator != (const EntityID &other) const;
		[[nodiscard]] bool operator < (const EntityID &other) const;
		[[nodiscard]] bool IsValid() const;
	};

	class EntityArchetype
	{
		ui64 _hash{};

	public:
		void Add(TypeId type);
		void Subtract(TypeId type);
	};

    class Entity
    {
	protected:
        void *_systemDataPointrs[MaxSystemDataPointers]{};
        std::list<void *> _extraSystemDataPointers{};
		EntityArchetype _archetype{};
        Entity *_parent = nullptr;
        vector<unique_ptr<Component>> _components{};
		EntityID _id = ui32_max;
		string _name{};
		bool _isEnabled = true;

		void AddComponent(unique_ptr<Component> component);
		void RemoveComponent(const Component &component);

    public:
		[[nodiscard]] const vector<unique_ptr<const Component>> &Components() const;
		[[nodiscard]] const string &Name() const;
		[[nodiscard]] const Entity *Parent() const;
		[[nodiscard]] EntityID ID() const;
		[[nodiscard]] EntityArchetype Archetype() const;
		[[nodiscard]] bool IsEnabledSelf() const;
		[[nodiscard]] bool IsEnabledInHierarchy() const;
    };
}