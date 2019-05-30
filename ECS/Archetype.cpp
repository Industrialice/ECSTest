#include "PreHeader.hpp"
#include "Archetype.hpp"

using namespace ECSTest;

// ArchetypeShort

ui64 Archetype::Hash() const
{
    return _u.typePart;
}

Archetype ECSTest::Archetype::FromFull(const ArchetypeFull &source)
{
    return source.ToShort();
}

bool ECSTest::Archetype::operator == (const Archetype &other) const
{
    bool equalTest = _u.typePart == other._u.typePart;
#ifdef CHECK_TYPES_IN_ARCHETYPES
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
    return _u.typePart < other._u.typePart;
}

// Archetype

ui64 ArchetypeFull::Hash() const
{
    return ((ui64)_u.idPart << 32) | _u.typePart;
}

Archetype ArchetypeFull::ToShort() const
{
    Archetype result;
    result._u.typePart = _u.typePart;
#ifdef CHECK_TYPES_IN_ARCHETYPES
    result._storedTypes = _storedTypes;
#endif
    return result;
}

bool ArchetypeFull::operator == (const ArchetypeFull &other) const
{
    bool equalTest = _u.typePart == other._u.typePart && _u.idPart == other._u.idPart;
#ifdef CHECK_TYPES_IN_ARCHETYPES
    if (equalTest)
    {
        ASSUME(std::equal(_storedTypes.begin(), _storedTypes.end(), other._storedTypes.begin(), other._storedTypes.end()));
        ASSUME(std::equal(_storedTypesFull.begin(), _storedTypesFull.end(), other._storedTypesFull.begin(), other._storedTypesFull.end()));
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
    return std::tie(_u.typePart, _u.idPart) < std::tie(other._u.typePart, other._u.idPart);
}