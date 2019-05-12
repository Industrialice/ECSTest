#pragma once

#include "EntityID.hpp"
#include "Array.hpp"

namespace ECSTest
{
    class NOVTABLE IEntitiesStream
    {
    public:
        struct ComponentDesc : ComponentDescription
        {
			const ui8 *data{}; // not aligned
        };

        struct StreamedEntity
        {
            EntityID entityId{};
            Array<ComponentDesc> components{};
        };

        virtual ~IEntitiesStream() = default;
        [[nodiscard]] virtual optional<StreamedEntity> Next() = 0; // the previous value may get invalidated when you request the next one
    };
}