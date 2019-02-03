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
	return _id != invalidId;
}

EntityID EntityIDGenerator::Generate()
{
    EntityID id = _current.load();
    _current.fetch_add(1);
    return id;
}

EntityID EntityIDGenerator::LastGenerated() const
{
    return _current.load();
}

EntityIDGenerator::EntityIDGenerator(EntityIDGenerator &&source) : _current{source._current.load()}
{}

EntityIDGenerator &EntityIDGenerator::operator = (EntityIDGenerator &&source)
{
    ASSUME(this != &source);
    _current.store(source._current.load());
    return *this;
}
