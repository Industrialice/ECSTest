#pragma once

#include <EntitiesStream.hpp>

namespace ECSTest
{
    class EntitiesStream : public IEntitiesStream
    {
    public:
        struct EntityData
        {
            StreamedEntity streamed;
            vector<ComponentDesc> descs;
            vector<unique_ptr<ui8[]>> componentsData;

            EntityData() = default;
            EntityData(EntityData &&) = default;
            EntityData &operator = (EntityData &&) = default;

            template <typename T> void AddComponent(const T &component)
            {
                EntitiesStream::ComponentDesc desc;
                desc.alignmentOf = alignof(T);
                desc.isUnique = T::IsUnique();
                desc.isTag = T::IsTag();
                desc.sizeOf = sizeof(T);
                desc.type = T::GetTypeId();
                if constexpr (T::IsTag() == false)
                {
                    auto componentData = make_unique<ui8[]>(sizeof(T));
                    memcpy(componentData.get(), &component, sizeof(T));
                    desc.data = componentData.get();
                    componentsData.emplace_back(move(componentData));
                }
                descs.emplace_back(desc);
            }
        };

    private:
        vector<EntityData> _entities{};
        uiw _currentEntity{};

    public:
        [[nodiscard]] virtual optional<StreamedEntity> Next() override
        {
            if (_currentEntity < _entities.size())
            {
                uiw index = _currentEntity++;
                return _entities[index].streamed;
            }
            return {};
        }

        void AddEntity(EntityID id, EntityData &&entity)
        {
            _entities.emplace_back(move(entity));
            _entities.back().streamed.entityId = id;
            _entities.back().streamed.components = ToArray(_entities.back().descs);
        }
    };
}