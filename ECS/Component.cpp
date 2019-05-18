#include "PreHeader.hpp"
#include "Component.hpp"

using namespace ECSTest;

ComponentID::ComponentID(ui32 id) : _id(id)
{}

ui32 ComponentID::ID() const
{
    return _id;
}

bool ComponentID::IsValid() const
{
    return _id != invalidId;
}

bool ComponentID::operator == (const ComponentID &other) const
{
    return _id == other._id;
}

bool ComponentID::operator != (const ComponentID &other) const
{
    return _id != other._id;
}

bool ComponentID::operator < (const ComponentID &other) const
{
    return _id < other._id;
}

bool ComponentID::operator <= (const ComponentID &other) const
{
	return _id <= other._id;
}

bool ComponentID::operator > (const ComponentID &other) const
{
	return _id > other._id;
}

bool ComponentID::operator >= (const ComponentID &other) const
{
	return _id >= other._id;
}

ComponentID::operator bool() const
{
	return IsValid();
}

ComponentID ComponentIDGenerator::Generate()
{
    auto id = ComponentID(_current.load());
    _current.fetch_add(1);
    return id;
}

ComponentID ComponentIDGenerator::LastGenerated() const
{
    return ComponentID(_current.load() - 1);
}

ComponentIDGenerator::ComponentIDGenerator(ComponentIDGenerator &&source) : _current(source._current.load())
{
}

ComponentIDGenerator &ComponentIDGenerator::operator = (ComponentIDGenerator &&source)
{
    ASSUME(this != &source);
    _current.store(source._current.load());
    return *this;
}
