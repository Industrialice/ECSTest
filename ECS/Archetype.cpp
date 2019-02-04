#include "PreHeader.hpp"
#include "Archetype.hpp"

using namespace ECSTest;

// ArchetypeShort

ui64 Archetype::Hash() const
{
    return _parted.typePart;
}

Archetype ECSTest::Archetype::FromFull(const ArchetypeFull &source)
{
    return source.ToShort();
}

bool ECSTest::Archetype::operator == (const Archetype &other) const
{
    return _whole == other._whole;
}

bool ECSTest::Archetype::operator != (const Archetype &other) const
{
    return _whole != other._whole;
}

bool ECSTest::Archetype::operator < (const Archetype &other) const
{
    return _whole < other._whole;
}

// Archetype

ui64 ArchetypeFull::Hash() const
{
    return _whole;
}

Archetype ArchetypeFull::ToShort() const
{
    Archetype result;
    result._parted.typePart = _parted.typePart;
    return result;
}

bool ArchetypeFull::operator == (const ArchetypeFull &other) const
{
    return _whole == other._whole;
}

bool ArchetypeFull::operator != (const ArchetypeFull &other) const
{
    return _whole != other._whole;
}

bool ArchetypeFull::operator < (const ArchetypeFull &other) const
{
    return _whole < other._whole;
}