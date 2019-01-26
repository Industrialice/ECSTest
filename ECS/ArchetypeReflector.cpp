#include "PreHeader.hpp"
#include "ArchetypeReflector.hpp"

using namespace ECSTest;

static bool Satisfies(Array<const StableTypeId> value, Array<const pair<StableTypeId, RequirementForComponent>> request);

bool ArchetypeReflector::Contains(Archetype archetype) const
{
	_lock.Lock(DIWRSpinLock::LockType::Read);
	bool contains = _library.find(archetype) != _library.end();
	_lock.Unlock(DIWRSpinLock::LockType::Read);
	return contains;
}

void ArchetypeReflector::AddToLibrary(Archetype archetype, vector<StableTypeId> &&types)
{
	_lock.Lock(DIWRSpinLock::LockType::Exclusive);

    // try to add to the archetype library, there's a chance such archetype is already registered
    auto [it, result] = _library.insert({archetype, move(types)});
    if (result) // added a new key
    {
        // check for matching archetypes
        for (auto &[key, value] : _matchingRequirementArchetypes)
        {
            if (Satisfies(ToArray(it->second), ToArray(key)))
            {
                ASSUME(std::find(value.begin(), value.end(), archetype) == value.end());
                value.push_back(archetype);
            }
        }
    }
	
    _lock.Unlock(DIWRSpinLock::LockType::Exclusive);
}

Array<const StableTypeId> ArchetypeReflector::Reflect(Archetype archetype) const
{
	_lock.Lock(DIWRSpinLock::LockType::Read);
	auto it = _library.find(archetype);
	ASSUME(it != _library.end());
	auto types = ToArray(it->second);
	_lock.Unlock(DIWRSpinLock::LockType::Read);
	return types;
}

void ArchetypeReflector::StartTrackingMatchingArchetypes(uiw id, Array<const pair<StableTypeId, RequirementForComponent>> types)
{
    _lock.Lock(DIWRSpinLock::LockType::Exclusive);

    // removing RequirementForComponent::Optional components, they don't affect the search
    auto filteredStorage = ALLOCA_TYPED(types.size(), decltype(types)::ItemType);
    uiw filteredCount = 0;
    for (auto &value : types)
    {
        if (value.second != RequirementForComponent::Optional)
        {
            filteredStorage[filteredCount++] = value;
        }
    }
    auto filtered = ToArray(filteredStorage, filteredCount);

    vector<Archetype> *ref = nullptr;

    auto existingSearch = _matchingRequirementArchetypes.find(filtered);
    if (existingSearch != _matchingRequirementArchetypes.end())
    {
        ref = &existingSearch->second;
    }
    else
    {
        vector<pair<StableTypeId, RequirementForComponent>> filteredVector = {filtered.begin(), filtered.end()};
        vector<Archetype> matchingArchetypes;
        for (const auto &[key, value] : _library)
        {
            if (Satisfies(ToArray(value), filtered))
            {
                matchingArchetypes.push_back(key);
            }
        }
        auto [insertKey, insertResult] = _matchingRequirementArchetypes.insert({move(filteredVector), move(matchingArchetypes)});
        ASSUME(insertResult);
        ref = &insertKey->second;
    }

    auto[insertKey, insertResult] = _matchingIDArchetypes.insert({id, ref});
    ASSUME(insertResult);

    _lock.Unlock(DIWRSpinLock::LockType::Exclusive);
}

void ArchetypeReflector::StopTrackingMatchingArchetypes(uiw id)
{
    _lock.Lock(DIWRSpinLock::LockType::Exclusive);

    auto it = _matchingIDArchetypes.find(id);
    ASSUME(it != _matchingIDArchetypes.end());
    auto *ref = it->second;
    _matchingIDArchetypes.erase(it);

    // if it was the last reference, remove the types as well
    bool isFound = false;
    for (const auto &[key, value] : _matchingIDArchetypes)
    {
        if (value == ref)
        {
            isFound = true;
            break;
        }
    }
    if (!isFound)
    {
        for (auto match = _matchingRequirementArchetypes.begin(); match != _matchingRequirementArchetypes.end(); ++match)
        {
            if (&match->second == ref)
            {
                _matchingRequirementArchetypes.erase(match);
                break;
            }
        }
    }

    _lock.Unlock(DIWRSpinLock::LockType::Exclusive);
}

const vector<Archetype> &ArchetypeReflector::FindMatchingArchetypes(uiw id) const
{
    _lock.Lock(DIWRSpinLock::LockType::Read);

    const auto &ref = *_matchingIDArchetypes.find(id)->second;

    _lock.Unlock(DIWRSpinLock::LockType::Read);

    return ref;
}

bool Satisfies(Array<const StableTypeId> value, Array<const pair<StableTypeId, RequirementForComponent>> request)
{
    for (auto[type, requirement] : request)
    {
        bool isFound = value.find(type) != value.end();

        if (requirement == RequirementForComponent::Required)
        {
            if (!isFound)
            {
                return false;
            }
        }
        else
        {
            ASSUME(requirement == RequirementForComponent::Subtractive);
            if (isFound)
            {
                return false;
            }
        }
    }
    return true;
}