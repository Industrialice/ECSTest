#pragma once

#include "Array.hpp"
#include "Archetype.hpp"
#include "DIWRSpinLock.hpp"
#include "Component.hpp"
#include <TypeIdentifiable.hpp>

namespace ECSTest
{
	class ArchetypeReflector
	{
        struct MatchingRequirementComparator
        {
            using is_transparent = void;

            bool operator () (const vector<pair<StableTypeId, RequirementForComponent>> &left, const vector<pair<StableTypeId, RequirementForComponent>> &right) const
            {
                return left < right;
            }

            bool operator () (const vector<pair<StableTypeId, RequirementForComponent>> &left, const Array<pair<StableTypeId, RequirementForComponent>> &right) const
            {
                return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
            }

            bool operator () (const Array<pair<StableTypeId, RequirementForComponent>> &left, const vector<pair<StableTypeId, RequirementForComponent>> &right) const
            {
                return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
            }
        };

		mutable DIWRSpinLock _lock{};
		std::unordered_map<Archetype, vector<StableTypeId>> _library{};
        std::unordered_map<uiw, vector<Archetype> *> _matchingIDArchetypes{};
        std::map<vector<pair<StableTypeId, RequirementForComponent>>, vector<Archetype>, MatchingRequirementComparator> _matchingRequirementArchetypes{};

	public:
		bool Contains(Archetype archetype) const;
		void AddToLibrary(Archetype archetype, vector<StableTypeId> &&types);
		Array<const StableTypeId> Reflect(Archetype archetype) const;
        void StartTrackingMatchingArchetypes(uiw id, Array<const pair<StableTypeId, RequirementForComponent>> types);
        void StopTrackingMatchingArchetypes(uiw id);
        const vector<Archetype> &FindMatchingArchetypes(uiw id) const; // the reference is valid as long as you continue tracking that id
	};
}