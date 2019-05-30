#pragma once

#include <IEntitiesStream.hpp>

namespace ECSTest
{
    class EntitiesStream final : public IEntitiesStream
    {
    public:
        struct EntityData
        {
			EntityID entityId;
            vector<ComponentDesc> descs;
            vector<byte> componentsData;

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
					uiw offset = componentsData.size();
					componentsData.resize(offset + sizeof(T));
                    MemOps::Copy(componentsData.data() + offset, (byte *)&component, sizeof(T));
                }
                descs.emplace_back(desc);
            }
        };

    private:
        vector<EntityData> _entities{};
        uiw _currentEntity{};

    public:
		void HintTotal(uiw count)
		{
			_entities.reserve(count);
		}

        [[nodiscard]] virtual optional<StreamedEntity> Next() override
        {
            if (_currentEntity < _entities.size())
            {
                uiw index = _currentEntity++;
				StreamedEntity entity = {_entities[index].entityId, ToArray(_entities[index].descs)};
                return entity;
            }
            return {};
        }

        void AddEntity(EntityID id, EntityData &&entity)
        {
            _entities.emplace_back(move(entity));

			auto &entry = _entities.back();

			entry.entityId = id;

			uiw offset = 0;
			for (auto &desc : entry.descs)
			{
				if (!desc.isTag)
				{
					desc.data = entry.componentsData.data() + offset;
					offset += desc.sizeOf;
				}
			}
        }
    };
}