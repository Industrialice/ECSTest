#include "PreHeader.hpp"
#include "Archetype.hpp"

using namespace ECSTest;

// ArchetypeShort

void Archetype::Add(StableTypeId type)
{
    _hash ^= type.Hash() & MainPartMask;
}

void Archetype::Subtract(StableTypeId type)
{
    Add(type);
}

ui64 Archetype::Hash() const
{
    return _hash;
}

Archetype ECSTest::Archetype::FromFull(const ArchetypeFull &source)
{
    return source.ToShort();
}

bool ECSTest::Archetype::operator == (const Archetype &other) const
{
    return _hash == other._hash;
}

bool ECSTest::Archetype::operator != (const Archetype &other) const
{
    return _hash != other._hash;
}

bool ECSTest::Archetype::operator < (const Archetype &other) const
{
    return _hash < other._hash;
}


// Archetype

void ArchetypeFull::Add(StableTypeId type, ui32 componentID)
{
    _hash ^= (type.Hash() & Archetype::MainPartMask) + ((ui64)componentID << ExtraPartStartBit);
}

void ArchetypeFull::Subtract(StableTypeId type, ui32 componentID)
{
    Add(type, componentID);
}

ui64 ArchetypeFull::Hash() const
{
    return _hash;
}

Archetype ArchetypeFull::ToShort() const
{
    Archetype result;
    result._hash = _hash & Archetype::MainPartMask;
    return result;
}

bool ArchetypeFull::operator == (const ArchetypeFull &other) const
{
    return _hash == other._hash;
}

bool ArchetypeFull::operator != (const ArchetypeFull &other) const
{
    return _hash != other._hash;
}

bool ArchetypeFull::operator < (const ArchetypeFull &other) const
{
    return _hash < other._hash;
}