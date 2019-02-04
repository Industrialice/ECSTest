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
    bool equalTest = _whole == other._whole;
#ifdef DEBUG
    if (equalTest)
    {
        ASSUME(std::equal(_storedTypes.begin(), _storedTypes.end(), other._storedTypes.begin(), other._storedTypes.end()));
    }
#endif
    return equalTest;
}

bool ECSTest::Archetype::operator != (const Archetype &other) const
{
    return !this->operator == (other);
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
#ifdef DEBUG
    result._storedTypes = _storedTypes;
#endif
    return result;
}

bool ArchetypeFull::operator == (const ArchetypeFull &other) const
{
    bool equalTest = _whole == other._whole;
#ifdef DEBUG
    if (equalTest)
    {
        ASSUME(std::equal(_storedTypes.begin(), _storedTypes.end(), other._storedTypes.begin(), other._storedTypes.end()));
    }
#endif
    return equalTest;
}

bool ArchetypeFull::operator != (const ArchetypeFull &other) const
{
    return !this->operator == (other);
}

bool ArchetypeFull::operator < (const ArchetypeFull &other) const
{
    return _whole < other._whole;
}