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
		ui32 id = 0;
	};

    class MessageStreamEntityAdded
    {
        friend class SystemsManager;

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
        shared_ptr<const vector<EntityWithComponents>> _source{};
        Archetype _archetype{};

        MessageStreamEntityAdded(Archetype archetype, const shared_ptr<const vector<EntityWithComponents>> &source) : _archetype(archetype), _source(source)
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
    };

	class MessageStreamEntityRemoved
	{
        friend class SystemsManager;

        shared_ptr<const vector<EntityID>> _source{};
        Archetype _archetype{};

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
        friend class SystemsManager;
        friend class MessageBuilder;

        std::unordered_map<Archetype, shared_ptr<vector<MessageStreamEntityAdded::EntityWithComponents>>> _data{};
    };

    class MessageStreamsBuilderEntityRemoved
    {
		friend class SystemsManager;
		friend class MessageBuilder;

		std::unordered_map<Archetype, shared_ptr<vector<EntityID>>> _data{};
    };

    class MessageBuilder
    {
		friend class SystemsManager;

        bool IsEmpty() const;
        void Clear();
		void Flush();
        [[nodiscard]] MessageStreamsBuilderEntityAdded &EntityAddedStreams();
        [[nodiscard]] MessageStreamsBuilderEntityRemoved &EntityRemovedStreams();

    public:
        class ComponentArrayBuilder
        {
			friend class MessageBuilder;

			Archetype _archetype{};
			vector<SerializedComponent> _components{};
			vector<ui8> _data{};

			void Clear();

        public:
            ComponentArrayBuilder &AddComponent(const EntitiesStream::ComponentDesc &desc, ui32 id); // the data will be copied over
            ComponentArrayBuilder &AddComponent(const SerializedComponent &sc); // the data will be copied over

            template <typename T> ComponentArrayBuilder &AddComponent(const T &component, ui32 id = 0)
            {
                SerializedComponent sc;
                sc.alignmentOf = alignof(T);
                sc.sizeOf = sizeof(T);
                sc.isUnique = T::IsUnique();
                sc.type = T::GetTypeId();
                sc.data = &component;
                sc.id = id;
                return AddComponent(sc);
            }
        };

        ComponentArrayBuilder &EntityAdded(EntityID entityID); // archetype will be computed after all the components were added, you can ignore the returned value if you don't want to add any components
        void EntityRemoved(Archetype archetype, EntityID entityID);
    
	private:
		ComponentArrayBuilder _cab{};
		MessageStreamsBuilderEntityRemoved _entityRemovedStream{};
		MessageStreamsBuilderEntityAdded _entityAddedStream{};
		EntityID _currentEntityId{};
	};
}