#pragma once

#include "EntityID.hpp"
#include "Archetype.hpp"
#include "EntitiesStream.hpp"

namespace ECSTest
{
	struct SerializedComponent
	{
		StableTypeId type{};
		ui16 sizeOf{};
		ui16 alignmentOf{};
		const ui8 *data{}; // aigned by alignmentOf
		bool isUnique{};
		ComponentID id{};
	};

    class MessageStreamEntityAdded
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;

    public:
        struct EntityWithComponents
        {
            friend class MessageBuilder;

            EntityID entityID;
            vector<SerializedComponent> components;

        private:
            vector<ui8> componentsData;
        };

    private:
        shared_ptr<vector<EntityWithComponents>> _source{};
        Archetype _archetype;

        MessageStreamEntityAdded(Archetype archetype, const shared_ptr<vector<EntityWithComponents>> &source) : _archetype(archetype), _source(source)
        {
            ASSUME(source->size());
        }

    public:
        [[nodiscard]] const EntityWithComponents *begin() const
        {
            return _source->data();
        }

        [[nodiscard]] const EntityWithComponents *end() const
        {
            return _source->data() + _source->size();
        }

        [[nodiscard]] Archetype Archetype() const
        {
            return _archetype;
        }

    private:
        [[nodiscard]] EntityWithComponents *begin()
        {
            return _source->data();
        }

        [[nodiscard]] EntityWithComponents *end()
        {
            return _source->data() + _source->size();
        }
    };

    class MessageStreamComponentChanged
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;
        friend class MessageBuilder;
        friend class MessageStreamsBuilderComponentChanged;

    public:
        struct ComponentInfo
        {
            friend class MessageBuilder;

            EntityID entityID;
            SerializedComponent component; // TODO: huge overkill, the only fields that differ between different components are data and id
        };

    private:
        struct InfoWithData
        {
            vector<ComponentInfo> infos;
            unique_ptr<ui8[], AlignedMallocDeleter> data;
        };

        shared_ptr<InfoWithData> _source{};
        StableTypeId _type{};

        MessageStreamComponentChanged(StableTypeId type, const shared_ptr<InfoWithData> &source) : _type(type), _source(source)
        {
            ASSUME(source->infos.size());
        }

    public:
        [[nodiscard]] const ComponentInfo *begin() const
        {
            return _source->infos.data();
        }

        [[nodiscard]] const ComponentInfo *end() const
        {
            return _source->infos.data() + _source->infos.size();
        }

        [[nodiscard]] StableTypeId Type() const
        {
            return _type;
        }

    /*private:
        [[nodiscard]] ComponentInfo *begin()
        {
            return _source->infos.data();
        }

        [[nodiscard]] ComponentInfo *end()
        {
            return _source->infos.data() + _source->infos.size();
        }*/
    };

	class MessageStreamEntityRemoved
	{
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;

        shared_ptr<const vector<EntityID>> _source{};
        Archetype _archetype;

		MessageStreamEntityRemoved(Archetype archetype, const shared_ptr<const vector<EntityID>> &source) : _archetype(archetype), _source(source)
		{
            ASSUME(source->size());
        }

	public:
        [[nodiscard]] const EntityID *begin() const
        {
            return _source->data();
        }

        [[nodiscard]] const EntityID *end() const
        {
            return _source->data() + _source->size();
        }

        [[nodiscard]] Archetype Archetype() const
        {
            return _archetype;
        }
	};

    class MessageStreamsBuilderEntityAdded
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;
        friend UnitTests;

        std::unordered_map<Archetype, shared_ptr<vector<MessageStreamEntityAdded::EntityWithComponents>>> _data{};
    };

    class MessageStreamsBuilderComponentChanged
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;
        friend UnitTests;

        std::unordered_map<StableTypeId, shared_ptr<MessageStreamComponentChanged::InfoWithData>> _data{};
    };

    class MessageStreamsBuilderEntityRemoved
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
		friend class MessageBuilder;
        friend UnitTests;

		std::unordered_map<Archetype, shared_ptr<vector<EntityID>>> _data{};
    };

    class MessageBuilder
    {
		friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;

        bool IsEmpty() const;
        void Clear();
		void Flush();
        [[nodiscard]] MessageStreamsBuilderEntityAdded &EntityAddedStreams();
        [[nodiscard]] MessageStreamsBuilderComponentChanged &ComponentChangedStreams();
        [[nodiscard]] MessageStreamsBuilderEntityRemoved &EntityRemovedStreams();

    public:
        class ComponentArrayBuilder
        {
			friend class MessageBuilder;

			vector<SerializedComponent> _components{};
			vector<ui8> _data{};

			void Clear();

        public:
            ComponentArrayBuilder &AddComponent(const EntitiesStream::ComponentDesc &desc, ComponentID id); // the data will be copied over
            ComponentArrayBuilder &AddComponent(const SerializedComponent &sc); // the data will be copied over

			template <typename T> ComponentArrayBuilder &AddComponent(const T &component, ComponentID id = {})
            {
                SerializedComponent sc;
                sc.alignmentOf = alignof(T);
                sc.sizeOf = sizeof(T);
                sc.isUnique = T::IsUnique();
                sc.type = T::GetTypeId();
                sc.data = (ui8 *)&component;
                sc.id = id;
                return AddComponent(sc);
            }
        };

        template <typename T> void ComponentChanged(EntityID entityID, const T &component, ComponentID id)
        {
            SerializedComponent sc;
            sc.alignmentOf = alignof(T);
            sc.sizeOf = sizeof(T);
            sc.isUnique = T::IsUnique();
            sc.type = T::GetTypeId();
            sc.data = (ui8 *)&component;
            sc.id = id;
            ComponentChanged(entityID, sc);
        }

        ComponentArrayBuilder &EntityAdded(EntityID entityID); // archetype will be computed after all the components were added, you can ignore the returned value if you don't want to add any components
        void ComponentChanged(EntityID entityID, const SerializedComponent &sc);
        void EntityRemoved(Archetype archetype, EntityID entityID);
    
	private:
		ComponentArrayBuilder _cab{};
		MessageStreamsBuilderEntityAdded _entityAddedStreams{};
        MessageStreamsBuilderComponentChanged _componentChangedStreams{};
        MessageStreamsBuilderEntityRemoved _entityRemovedStreams{};
		EntityID _currentEntityId{};
	};
}