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
            TypeId type{};
            ui16 sizeOf{};
            ui16 alignmentOf{};
            ui8 data[];
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