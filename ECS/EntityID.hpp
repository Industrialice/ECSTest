#pragma once

#include "Component.hpp"

namespace ECSTest
{
	class EntityID : public OpaqueID<ui32, ui32_max>
	{
		ui32 _hint = ui32_max;

	public:
		EntityID() = default;
		EntityID(ui32 id, ui32 hint) : OpaqueID(id), _hint(hint)
		{}

		ui32 Hint() const;
		ui32 Hash() const;
	};

    class EntityIDGenerator
    {
    protected:
        ui32 _currentId = 0;
		UniqueIdManager _hintGenerator{};

    public:
		[[nodiscard]] EntityID Generate();
		void Free(EntityID id); // use it when you're removing an entity from ECS manager to release the hint so it can be reused
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