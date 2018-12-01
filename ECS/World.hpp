#pragma once

#include "Entity.hpp"

namespace ECSTest
{
    class World
    {
    public:
        struct ComponentInfo
        {
            TypeId type{};
            ui32 sizeOf{};
            ui32 alignmentOf{};
            unique_ptr<ui8[], AlignedMallocDeleter> component{};
        };

    private:
        friend class SystemsManager;

        struct EntityEntry
        {
            Entity entity{};
            vector<ComponentInfo> components{};
        };

        vector<EntityEntry> _entities{};

    public:
        void AddEntity(Entity &&entity, vector<ComponentInfo> &&components);
    };
}