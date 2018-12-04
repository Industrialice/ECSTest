#include "PreHeader.hpp"
#include "EntityID.hpp"

using namespace ECSTest;

EntityID::EntityID(ui32 id) : _id(id)
{
}

ui32 EntityID::Hash() const
{
	return _id;
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