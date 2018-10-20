#include "PreHeader.hpp"
#include "Entity.hpp"

using namespace ECSTest;

// EntityID

EntityID::EntityID(ui32 id) : _id(id)
{
}

bool EntityID::operator == (const EntityID &other) const
{
	return _id == other._id;
}

bool EntityID::operator != (const EntityID &other) const
{
	return _id != other._id;
}

bool EntityID::operator < (const EntityID &other) const
{
	return _id < other._id;
}

bool EntityID::IsValid() const
{
	return _id != ui32_max;
}

// Entity

void Entity::AddComponent(unique_ptr<Component> component)
{
    _components.push_back(move(component));
}

void Entity::RemoveComponent(const Component &component)
{
	auto pred = [&component](const unique_ptr<Component> &contained)
	{
		return contained.get() == &component;
	};
	auto it = std::find_if(_components.begin(), _components.end(), pred);
	if (it == _components.end())
	{
		SOFTBREAK;
		return;
	}
	_components.erase(it);
}

const vector<unique_ptr<const Component>> &Entity::Components() const
{
    return (vector<unique_ptr<const Component>> &)_components;
}

const string &Entity::Name() const
{
	return _name;
}

const Entity *Entity::Parent() const
{
	return _parent;
}

EntityID Entity::ID() const
{
	ASSUME(_id != ui32_max);
	return _id;
}

bool Entity::IsEnabledSelf() const
{
	return _isEnabled;
}

bool Entity::IsEnabledInHierarchy() const
{
	if (_isEnabled == false)
	{
		return false;
	}
	if (_parent)
	{
		return _parent->IsEnabledInHierarchy();
	}
	return true;
}