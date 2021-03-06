#pragma once

#include "Array.hpp"
#include "Archetype.hpp"
#include "Component.hpp"

namespace ECSTest
{
	class ArchetypeReflector
	{
        struct MatchingRequirementComparator
        {
            using is_transparent = void;

			bool operator () (const vector<ArchetypeDefiningRequirement> &left, const vector<ArchetypeDefiningRequirement> &right) const;
			bool operator () (const vector<ArchetypeDefiningRequirement> &left, const Array<const ArchetypeDefiningRequirement> &right) const;
			bool operator () (const Array<const ArchetypeDefiningRequirement> &left, const vector<ArchetypeDefiningRequirement> &right) const;
        };

		DIWRSpinLock _lock{};
		std::unordered_map<Archetype, vector<TypeId>> _library{}; // all currently stored archetypes with component ids that compose them
        std::unordered_map<uiw, vector<Archetype> *> _matchingIDArchetypes{}; // maps systems to lists of archetypes that satisfy their requirements
        std::map<vector<ArchetypeDefiningRequirement>, vector<Archetype>, MatchingRequirementComparator> _matchingRequirementArchetypes{}; // mapping each requirement to the list of archetypes all of whom satisfy it

	public:
        [[nodiscard]] bool Contains(const Archetype &archetype) const;
		void AddToLibrary(const Archetype &archetype, vector<TypeId> &&types);
        [[nodiscard]] Array<const TypeId> Reflect(const Archetype &archetype) const;
        void StartTrackingMatchingArchetypes(uiw id, Array<const ArchetypeDefiningRequirement> archetypeDefining);
        void StopTrackingMatchingArchetypes(uiw id);
        [[nodiscard]] const vector<Archetype> &FindMatchingArchetypes(uiw id) const; // the reference is valid as long as you continue tracking that id
        [[nodiscard]] static bool Satisfies(Array<const TypeId> value, Array<const ArchetypeDefiningRequirement> request);
	};
}