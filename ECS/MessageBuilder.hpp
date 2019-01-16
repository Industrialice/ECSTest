#pragma once

#include "EntityID.hpp"
#include "Archetype.hpp"
#include "EntitiesStream.hpp"

namespace ECSTest
{
    class MessageStreamEntityRemoved
    {
    public:
    };

    class MessageStreamEntityAdded
    {
    public:
    };

    class MessageBuilder
    {
    public:
        struct SerializedComponent
        {
            StableTypeId type{};
            ui16 sizeOf{};
            ui16 alignmentOf{};
            const ui8 *data{};
            bool isUnique{};
            ui32 id = 0;
        };

        class ComponentArrayBuilder
        {
        public:
            ComponentArrayBuilder &AddComponent(const EntitiesStream::ComponentDesc &desc, ui32 id); // the data will be copied over
            ComponentArrayBuilder &AddComponent(const SerializedComponent &serializedComponent); // the data will be copied over

            template <typename T> ComponentArrayBuilder &AddComponent(const T &component, ui32 id = 0)
            {
                SerializedComponent desc;
                desc.alignmentOf = alignof(T);
                desc.sizeOf = sizeof(T);
                desc.isUnique = T::IsUnique();
                desc.type = T::GetTypeId();
                desc.data = &component;
                desc.id = id;
                return AddComponent(desc);
            }
        };

        void EntityRemoved(ArchetypeShort archetype, EntityID entityID);
        ComponentArrayBuilder &EntityAdded(EntityID entityID); // archetype will be computed after all the components were added
    };
}