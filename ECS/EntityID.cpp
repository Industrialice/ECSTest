#include "PreHeader.hpp"
#include "EntityID.hpp"

using namespace ECSTest;

ui32 EntityID::Hint() const
{
	return _hint;
}

ui32 EntityID::Hash() const
{
	return _id;
}

EntityID EntityIDGenerator::Generate()
{
    auto id = EntityID(_currentId, _hintGenerator.Allocate());
	_currentId += 1;
    return id;
}

void EntityIDGenerator::Free(EntityID id)
{
	_hintGenerator.Free(id.Hint());
}

EntityIDGenerator::EntityIDGenerator(EntityIDGenerator &&source) noexcept : _currentId{source._currentId}, _hintGenerator(move(source._hintGenerator))
{}

EntityIDGenerator &EntityIDGenerator::operator = (EntityIDGenerator &&source) noexcept
{
    ASSUME(this != &source);
	_currentId = source._currentId;
	_hintGenerator = move(source._hintGenerator);
    return *this;
}
