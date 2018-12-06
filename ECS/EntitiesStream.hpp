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
			ui8 *data{};
            pair<const StableTypeId *, uiw> excludes{}; // assumed to point at static memory
        };

        struct StreamedEntity
        {
            EntityID entityId{};
            Array<ComponentDesc> components{};
        };

        virtual ~EntitiesStream() = default;
        [[nodiscard]] virtual optional<StreamedEntity> Next() = 0;
    };
}