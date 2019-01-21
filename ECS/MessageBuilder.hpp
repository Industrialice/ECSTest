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

	class MessageStreamEntityRemoved
	{
		friend class SystemsManager;

		const vector<EntityID> &_source;
		uiw _current = 0;

		MessageStreamEntityRemoved(const vector<EntityID> &source) : _source(source)
		{}

		void Rewind();

	public:
		optional<EntityID> Next();
	};

	class MessageStreamEntityAdded
	{
		friend class SystemsManager;
		friend class MessageStreamsBuilderEntityAdded;
		friend class MessageBuilder;

		struct EntityWithComponents
		{
			EntityID entityID;
			vector<SerializedComponent> components;
			vector<ui8> componentsData;
		};

		const vector<EntityWithComponents> &_source;
		uiw _current = 0;

		MessageStreamEntityAdded(const vector<EntityWithComponents> &source) : _source(source)
		{}

		void Rewind();

	public:
		struct EntityAndComponents
		{
			EntityID entityID;
			Array<const SerializedComponent> components;
		};

		optional<EntityAndComponents> Next();
	};

    class MessageStreamsBuilderEntityRemoved
    {
		friend class SystemsManager;
		friend class MessageBuilder;

		std::unordered_map<Archetype, vector<EntityID>> _data{};
    };

    class MessageStreamsBuilderEntityAdded
    {
		friend class SystemsManager;
		friend class MessageBuilder;

		std::unordered_map<Archetype, vector<MessageStreamEntityAdded::EntityWithComponents>> _data{};
    };

    class MessageBuilder
    {
		friend class SystemsManager;

		void Flush();
		MessageStreamsBuilderEntityRemoved &EntityRemovedStream();
		MessageStreamsBuilderEntityAdded &EntityAddedStream();

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

        void EntityRemoved(Archetype archetype, EntityID entityID);
        ComponentArrayBuilder &EntityAdded(EntityID entityID); // archetype will be computed after all the components were added
    
	private:
		ComponentArrayBuilder _cab{};
		MessageStreamsBuilderEntityRemoved _entityRemovedStream{};
		MessageStreamsBuilderEntityAdded _entityAddedStream{};
		EntityID _currentEntityId{};
	};
}