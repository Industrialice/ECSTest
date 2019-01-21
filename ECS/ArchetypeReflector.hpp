#pragma once

#include "Array.hpp"
#include "Archetype.hpp"
#include "DIWRSpinLock.hpp"
#include <TypeIdentifiable.hpp>

namespace ECSTest
{
	class ArchetypeReflector
	{
		mutable DIWRSpinLock _lock{};
		std::unordered_map<Archetype, vector<StableTypeId>> _library{};

	public:
		bool Contains(Archetype archetype) const;
		void AddToLibrary(Archetype archetype, vector<StableTypeId> &&types);
		Array<const StableTypeId> Reflect(Archetype archetype) const;
	};
}