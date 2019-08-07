#include "PreHeader.hpp"
#include "Component.hpp"

using namespace ECSTest;

ui32 ComponentID::ID() const
{
    return _id;
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

ComponentIDGenerator::ComponentIDGenerator(ComponentIDGenerator &&source) noexcept : _current(source._current.load())
{
}

ComponentIDGenerator &ComponentIDGenerator::operator = (ComponentIDGenerator &&source) noexcept
{
    ASSUME(this != &source);
    _current.store(source._current.load());
    return *this;
}
