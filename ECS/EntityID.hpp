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
        explicit EntityID(ui32 id);
		[[nodiscard]] ui32 Hash() const;
		[[nodiscard]] bool operator == (const EntityID &other) const;
		[[nodiscard]] bool operator != (const EntityID &other) const;
		[[nodiscard]] bool operator < (const EntityID &other) const;
		[[nodiscard]] bool IsValid() const;
	};

    class EntityIDGenerator
    {
    protected:
        std::atomic<ui32> _current = 0;

    public:
		[[nodiscard]] EntityID Generate();
		[[nodiscard]] EntityID LastGenerated() const;
        EntityIDGenerator() = default;
        EntityIDGenerator(EntityIDGenerator &&source);
        EntityIDGenerator &operator = (EntityIDGenerator &&source);
    };
}

namespace std
{
	template <> struct hash<ECSTest::EntityID>
	{
        [[nodiscard]] size_t operator()(const ECSTest::EntityID &value) const
		{
			return value.Hash();
		}
	};
}