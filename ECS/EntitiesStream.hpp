#pragma once

#include "EntityID.hpp"
#include "Array.hpp"

namespace ECSTest
{
    class EntitiesStream
    {
    public:
        struct ComponentDesc
        {
            StableTypeId type{};
            ui16 sizeOf{};
            ui16 alignmentOf{};
			const ui8 *data{};
            bool isUnique{};
        };

        struct StreamedEntity
        {
            EntityID entityId{};
            Array<ComponentDesc> components{};
        };

        virtual ~EntitiesStream() = default;
        [[nodiscard]] virtual optional<StreamedEntity> Next() = 0; // the previous value may get invalidated when you request the next one
    };
}