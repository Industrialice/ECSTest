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

void ArchetypeReflector::StartTrackingMatchingArchetypes(uiw id, Array<const ArchetypeDefiningRequirement> archetypeDefining)
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
        vector<ArchetypeDefiningRequirement> filteredVector = {archetypeDefining.begin(), archetypeDefining.end()};
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

bool ArchetypeReflector::Satisfies(Array<const StableTypeId> value, Array<const ArchetypeDefiningRequirement> request)
{
#ifdef DEBUG
	{
		ui8 currentGroup = 0;

		for (auto[type, group, requirement] : request)
		{
			if (requirement == RequirementForComponent::Required || requirement == RequirementForComponent::RequiredWithData || requirement == RequirementForComponent::Subtractive)
			{
				ASSUME(currentGroup == group || currentGroup + 1 == group); // groups increase must be gradual
			}
			else
			{
				HARDBREAK; // requirements that don't affect archetype aren't allowed
			}
			currentGroup = group;
		}
	}
#endif

	ui8 currentGroup = ui8_max;
	bool isGroupSatisfied = true;

    for (auto[type, group, requirement] : request)
    {
		bool isGroupChanging = currentGroup != group;
		if (isGroupChanging)
		{
			if (isGroupSatisfied == false)
			{
				return false;
			}
			isGroupSatisfied = false;
			currentGroup = group;
		}

        bool isFound = value.find(type) != value.end();

        if (requirement == RequirementForComponent::Required || requirement == RequirementForComponent::RequiredWithData)
        {
			if (isFound)
			{
				isGroupSatisfied = true;
			}
        }
        else
        {
            if (isFound)
            {
                return false;
            }
			isGroupSatisfied = true;
        }
    }

	return isGroupSatisfied;
}

namespace ECSTest
{
	constexpr bool operator < (const ArchetypeDefiningRequirement &left, const ArchetypeDefiningRequirement &right)
	{
		return std::tie(left.type, left.group, left.requirement) < std::tie(right.type, right.group, right.requirement);
	}
}

bool ArchetypeReflector::MatchingRequirementComparator::operator()(const vector<ArchetypeDefiningRequirement> &left, const vector<ArchetypeDefiningRequirement> &right) const
{
	return left < right;
}

bool ArchetypeReflector::MatchingRequirementComparator::operator()(const vector<ArchetypeDefiningRequirement> &left, const Array<const ArchetypeDefiningRequirement> &right) const
{
	return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

bool ArchetypeReflector::MatchingRequirementComparator::operator()(const Array<const ArchetypeDefiningRequirement> &left, const vector<ArchetypeDefiningRequirement> &right) const
{
	return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}
