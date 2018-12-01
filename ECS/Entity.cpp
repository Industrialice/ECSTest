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

// EntityArchetype

void EntityArchetype::Add(TypeId type)
{
	_hash ^= type.Hash();
}

void EntityArchetype::Subtract(TypeId type)
{
	_hash ^= type.Hash();
}

bool EntityArchetype::operator == (const EntityArchetype &other) const
{
	return _hash == other._hash;
}

bool EntityArchetype::operator != (const EntityArchetype &other) const
{
	return _hash != other._hash;
}

bool EntityArchetype::operator < (const EntityArchetype &other) const
{
	return _hash < other._hash;
}
