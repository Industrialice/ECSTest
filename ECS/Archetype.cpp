#include "PreHeader.hpp"
#include "Archetype.hpp"

using namespace ECSTest;

// ArchetypeShort

void ArchetypeShort::Add(TypeId type)
{
    _hash ^= type.Hash() & MainPartMask;
}

void ArchetypeShort::Subtract(TypeId type)
{
    Add(type);
}

ui64 ArchetypeShort::Hash() const
{
    return _hash;
}

ArchetypeShort ECSTest::ArchetypeShort::FromLong(const Archetype &source)
{
    return source.ToShort();
}

bool ECSTest::ArchetypeShort::operator == (const ArchetypeShort &other) const
{
    return _hash == other._hash;
}

bool ECSTest::ArchetypeShort::operator != (const ArchetypeShort &other) const
{
    return _hash != other._hash;
}

bool ECSTest::ArchetypeShort::operator < (const ArchetypeShort &other) const
{
    return _hash < other._hash;
}


// Archetype

void Archetype::Add(TypeId type, ui32 index)
{
    _hash ^= type.Hash() + ((ui64)index << ExtraPartStartBit);
}

void Archetype::Subtract(TypeId type, ui32 index)
{
    Add(type, index);
}

ui64 Archetype::Hash() const
{
    return _hash;
}

ArchetypeShort Archetype::ToShort() const
{
    ArchetypeShort result;
    result._hash = _hash & ArchetypeShort::MainPartMask;
    return result;
}

bool Archetype::operator == (const Archetype &other) const
{
    return _hash == other._hash;
}

bool Archetype::operator != (const Archetype &other) const
{
    return _hash != other._hash;
}

bool Archetype::operator < (const Archetype &other) const
{
    return _hash < other._hash;
}