#pragma once

#include <PreHeader.hpp>
#include <SystemsManager.hpp>

namespace ECSTest
{
    class TestEntities : public EntitiesStream
    {
    public:
        struct PreStreamedEntity
        {
            StreamedEntity streamed;
            vector<ComponentDesc> descs;
            vector<unique_ptr<ui8[]>> componentsData;

            PreStreamedEntity() = default;
            PreStreamedEntity(PreStreamedEntity &&) = default;
            PreStreamedEntity &operator = (PreStreamedEntity &&) = default;
        };

    private:
        vector<PreStreamedEntity> _entities{};
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

        void AddEntity(EntityID id, PreStreamedEntity &&entity)
        {
            _entities.emplace_back(move(entity));
            _entities.back().streamed.entityId = id;
        }
    };

    template <typename T> void StreamComponent(const T &component, TestEntities::PreStreamedEntity &preStreamed)
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
            preStreamed.componentsData.emplace_back(move(componentData));
        }
        preStreamed.descs.emplace_back(desc);
    }
}