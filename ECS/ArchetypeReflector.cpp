#include "PreHeader.hpp"
#include "ArchetypeReflector.hpp"

using namespace ECSTest;

bool ArchetypeReflector::Contains(Archetype archetype) const
{
	auto unlocker = _lock.Lock(DIWRSpinLock::LockType::Read);
	bool contains = _library.find(archetype) != _library.end();
    unlocker.Unlock();
	return contains;
}

void ArchetypeReflector::AddToLibrary(Archetype archetype, vector<StableTypeId> &&types)
{
#ifdef DEBUG
    ASSUME(std::equal(archetype._storedTypes.begin(), archetype._storedTypes.end(), types.begin(), types.end()));
#endif

	auto unlocker = _lock.Lock(DIWRSpinLock::LockType::Exclusive);

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
	
    unlocker.Unlock();
}

Array<const StableTypeId> ArchetypeReflector::Reflect(Archetype archetype) const
{
	auto unlocker = _lock.Lock(DIWRSpinLock::LockType::Read);
	auto it = _library.find(archetype);
	ASSUME(it != _library.end());
	auto types = ToArray(it->second);
    unlocker.Unlock();
#ifdef DEBUG
    ASSUME(std::equal(types.begin(), types.end(), archetype._storedTypes.begin(), archetype._storedTypes.end()));
#endif
	return types;
}

void ArchetypeReflector::StartTrackingMatchingArchetypes(uiw id, Array<const pair<StableTypeId, RequirementForComponent>> archetypeDefining)
{
    auto unlocker = _lock.Lock(DIWRSpinLock::LockType::Exclusive);

    vector<Archetype> *ref = nullptr;

    auto existingSearch = _matchingRequirementArchetypes.find(archetypeDefining);
    if (existingSearch != _matchingRequirementArchetypes.end())
    {
        ref = &existingSearch->second;
    }
    else
    {
        vector<pair<StableTypeId, RequirementForComponent>> filteredVector = {archetypeDefining.begin(), archetypeDefining.end()};
        vector<Archetype> matchingArchetypes;
        for (const auto &[key, value] : _library)
        {
            if (Satisfies(ToArray(value), archetypeDefining))
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

    unlocker.Unlock();
}

void ArchetypeReflector::StopTrackingMatchingArchetypes(uiw id)
{
    auto unlocker = _lock.Lock(DIWRSpinLock::LockType::Exclusive);

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

    unlocker.Unlock();
}

const vector<Archetype> &ArchetypeReflector::FindMatchingArchetypes(uiw id) const
{
    auto unlocker = _lock.Lock(DIWRSpinLock::LockType::Read);

    const auto &ref = *_matchingIDArchetypes.find(id)->second;

    unlocker.Unlock();

    return ref;
}

bool ArchetypeReflector::Satisfies(Array<const StableTypeId> value, Array<const pair<StableTypeId, RequirementForComponent>> request)
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