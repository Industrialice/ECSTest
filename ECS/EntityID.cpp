#include "PreHeader.hpp"
#include "EntityID.hpp"

using namespace ECSTest;

EntityID EntityIDGenerator::Generate()
{
    auto id = EntityID(_current.load());
    _current.fetch_add(1);
    return id;
}

EntityID EntityIDGenerator::LastGenerated() const
{
    return EntityID(_current.load() - 1);
}

EntityIDGenerator::EntityIDGenerator(EntityIDGenerator &&source) noexcept : _current{source._current.load()}
{}

EntityIDGenerator &EntityIDGenerator::operator = (EntityIDGenerator &&source) noexcept
{
    ASSUME(this != &source);
    _current.store(source._current.load());
    return *this;
}
