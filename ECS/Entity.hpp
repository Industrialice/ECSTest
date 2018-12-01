#pragma once

#include "Component.hpp"

namespace ECSTest
{
	class EntityID
	{
		ui32 _id = ui32_max;

	public:
        EntityID() = default;
		EntityID(ui32 id);
		[[nodiscard]] bool operator == (const EntityID &other) const;
		[[nodiscard]] bool operator != (const EntityID &other) const;
		[[nodiscard]] bool operator < (const EntityID &other) const;
		[[nodiscard]] bool IsValid() const;
	};

	class EntityArchetype
	{
		ui64 _hash{};

	public:
		void Add(TypeId type);
		void Subtract(TypeId type);
		[[nodiscard]] bool operator == (const EntityArchetype &other) const;
		[[nodiscard]] bool operator != (const EntityArchetype &other) const;
		[[nodiscard]] bool operator < (const EntityArchetype &other) const;
	};
}