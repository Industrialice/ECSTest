#pragma once

#include "Component.hpp"

namespace ECSTest
{
	class EntityID
	{
	public:
		static constexpr ui32 invalidId = ui32_max;

	private:
		ui32 _id = invalidId;

	public:
        EntityID() = default;
		EntityID(ui32 id);
		[[nodiscard]] ui32 Hash() const;
		[[nodiscard]] bool operator == (const EntityID &other) const;
		[[nodiscard]] bool operator != (const EntityID &other) const;
		[[nodiscard]] bool operator < (const EntityID &other) const;
		[[nodiscard]] bool IsValid() const;
	};
}

namespace std
{
	template <> struct hash<ECSTest::EntityID>
	{
		size_t operator()(const ECSTest::EntityID &value) const
		{
			return value.Hash();
		}
	};
}