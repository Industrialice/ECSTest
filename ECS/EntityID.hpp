#pragma once

#include "Component.hpp"

namespace ECSTest
{
	class EntityID : public OpaqueID<ui32, ui32_max>
	{
		using OpaqueID::OpaqueID;
	};

    class EntityIDGenerator
    {
    protected:
        std::atomic<ui32> _current = 0;

    public:
		[[nodiscard]] EntityID Generate();
		[[nodiscard]] EntityID LastGenerated() const;
        EntityIDGenerator() = default;
        EntityIDGenerator(EntityIDGenerator &&source) noexcept;
        EntityIDGenerator &operator = (EntityIDGenerator &&source) noexcept;
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