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
}