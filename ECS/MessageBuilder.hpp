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

        template <typename T> T &Cast()
        {
            ASSUME(T::GetTypeId() == type);
            return *(T *)data;
        }

        template <typename T> const T &Cast() const
        {
            ASSUME(T::GetTypeId() == type);
            return *(T *)data;
        }

        template <typename T> T *TryCast()
        {
            if (T::GetTypeId() == type)
            {
                return (T *)data;
            }
            return nullptr;
        }

        template <typename T> const T *TryCast() const
        {
            if (T::GetTypeId() == type)
            {
                return (T *)data;
            }
            return nullptr;
        }
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

            template <typename T> const T *FindComponent() const
            {
                for (auto &c : components)
                {
                    if (c.type == T::GetTypeId())
                    {
                        return (T *)c.data;
                    }
                }
                return nullptr;
            }

        private:
            vector<ui8> componentsData;
        };

    private:
        shared_ptr<vector<EntityWithComponents>> _source{};
        Archetype _archetype;
        string _sourceName{};

        MessageStreamEntityAdded(Archetype archetype, const shared_ptr<vector<EntityWithComponents>> &source, string_view sourceName) : _archetype(archetype), _source(source), _sourceName(sourceName)
        {
            ASSUME(_source->size());
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

        [[nodiscard]] const string &SourceName() const
        {
            return _sourceName;
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

    class MessageStreamComponentAdded
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;
        friend class MessageBuilder;
        friend class MessageStreamsBuilderComponentAdded;

    public:
        struct ComponentInfo
        {
            friend class MessageBuilder;

            EntityID entityID;
            SerializedComponent component;
        };

    private:
        struct InfoWithData
        {
            vector<ComponentInfo> infos;
            unique_ptr<ui8[], AlignedMallocDeleter> data;
        };

        shared_ptr<InfoWithData> _source{};
        StableTypeId _type{};
        string _sourceName{};

        MessageStreamComponentAdded(StableTypeId type, const shared_ptr<InfoWithData> &source, string_view sourceName) : _type(type), _source(source), _sourceName(sourceName)
        {
            ASSUME(_source->infos.size());
			ASSUME(_type != StableTypeId{});
        }

        [[nodiscard]] const string &SourceName() const
        {
            return _sourceName;
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
        string _sourceName{};

        MessageStreamComponentChanged(StableTypeId type, const shared_ptr<InfoWithData> &source, string_view sourceName) : _type(type), _source(source), _sourceName(sourceName)
        {
            ASSUME(_source->infos.size());
			ASSUME(_type != StableTypeId{});
        }

        [[nodiscard]] const string &SourceName() const
        {
            return _sourceName;
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
    };

    class MessageStreamComponentRemoved
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;
        friend class MessageBuilder;
        friend class MessageStreamsBuilderComponentRemoved;

    public:
        struct ComponentInfo
        {
            friend class MessageBuilder;

            EntityID entityID;
            ComponentID componentID;
        };

    private:
        shared_ptr<vector<ComponentInfo>> _source{};
        StableTypeId _type{};
        string _sourceName{};

        MessageStreamComponentRemoved(StableTypeId type, const shared_ptr<vector<ComponentInfo>> &source, string_view sourceName) : _type(type), _source(source), _sourceName(sourceName)
        {
            ASSUME(_source->size());
			ASSUME(_type != StableTypeId{});
        }

        [[nodiscard]] const string &SourceName() const
        {
            return _sourceName;
        }

    public:
        [[nodiscard]] const ComponentInfo *begin() const
        {
            return _source->data();
        }

        [[nodiscard]] const ComponentInfo *end() const
        {
            return _source->data() + _source->size();
        }

        [[nodiscard]] StableTypeId Type() const
        {
            return _type;
        }
    };

	class MessageStreamEntityRemoved
	{
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;

        shared_ptr<const vector<EntityID>> _source{};
        Archetype _archetype;
        string _sourceName{};

		MessageStreamEntityRemoved(Archetype archetype, const shared_ptr<const vector<EntityID>> &source, string_view sourceName) : _archetype(archetype), _source(source), _sourceName(sourceName)
		{
            ASSUME(source->size());
        }

        [[nodiscard]] const string &SourceName() const
        {
            return _sourceName;
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

    class MessageStreamsBuilderComponentAdded
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;
        friend UnitTests;

        std::unordered_map<StableTypeId, shared_ptr<MessageStreamComponentAdded::InfoWithData>> _data{};
    };

    class MessageStreamsBuilderComponentChanged
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;
        friend UnitTests;

        std::unordered_map<StableTypeId, shared_ptr<MessageStreamComponentChanged::InfoWithData>> _data{};
    };

    class MessageStreamsBuilderComponentRemoved
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;
        friend UnitTests;

        std::unordered_map<StableTypeId, shared_ptr<vector<MessageStreamComponentRemoved::ComponentInfo>>> _data{};
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

        void SourceName(string_view name);
        const string &SourceName() const;
        bool IsEmpty() const;
        void Clear();
		void Flush();
        [[nodiscard]] MessageStreamsBuilderEntityAdded &EntityAddedStreams();
        [[nodiscard]] MessageStreamsBuilderComponentAdded &ComponentAddedStreams();
        [[nodiscard]] MessageStreamsBuilderComponentChanged &ComponentChangedStreams();
        [[nodiscard]] MessageStreamsBuilderComponentRemoved &ComponentRemovedStreams();
        [[nodiscard]] MessageStreamsBuilderEntityRemoved &EntityRemovedStreams();
        [[nodiscard]] const vector<EntityID> &EntityRemovedNoArchetype();

    public:
        class ComponentArrayBuilder
        {
			friend class MessageBuilder;

			vector<SerializedComponent> _components{};
			vector<ui8> _data{};

			void Clear();

        public:
			ComponentArrayBuilder() = default;
			ComponentArrayBuilder(ComponentArrayBuilder &&) = default;
			ComponentArrayBuilder &operator = (ComponentArrayBuilder &&) = default;

            ComponentArrayBuilder &AddComponent(const EntitiesStream::ComponentDesc &desc, ComponentID id); // the data will be copied over
            ComponentArrayBuilder &AddComponent(const SerializedComponent &sc); // the data will be copied over

            template <typename T, typename = std::enable_if_t<T::IsUnique()>> ComponentArrayBuilder &AddComponent(const T &component)
            {
                SerializedComponent sc;
                sc.alignmentOf = alignof(T);
                sc.sizeOf = sizeof(T);
                sc.isUnique = T::IsUnique();
                sc.type = T::GetTypeId();
                sc.data = (ui8 *)&component;
                sc.id = {};
                return AddComponent(sc);
            }

			template <typename T, typename = std::enable_if_t<T::IsUnique() == false>> ComponentArrayBuilder &AddComponent(const T &component, ComponentID id)
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

        template <typename T, typename = std::enable_if_t<T::IsUnique()>> void ComponentAdded(EntityID entityID, const T &component)
        {
            SerializedComponent sc;
            sc.alignmentOf = alignof(T);
            sc.sizeOf = sizeof(T);
            sc.isUnique = T::IsUnique();
            sc.type = T::GetTypeId();
            sc.data = (ui8 *)&component;
            sc.id = {};
            ComponentAdded(entityID, sc);
        }

        template <typename T, typename = std::enable_if_t<T::IsUnique() == false>> void ComponentAdded(EntityID entityID, const T &component, ComponentID id)
        {
            SerializedComponent sc;
            sc.alignmentOf = alignof(T);
            sc.sizeOf = sizeof(T);
            sc.isUnique = T::IsUnique();
            sc.type = T::GetTypeId();
            sc.data = (ui8 *)&component;
            sc.id = id;
            ComponentAdded(entityID, sc);
        }

		template <typename T, typename = std::enable_if_t<T::IsUnique()>> void ComponentChanged(EntityID entityID, const T &component)
		{
			SerializedComponent sc;
			sc.alignmentOf = alignof(T);
			sc.sizeOf = sizeof(T);
			sc.isUnique = T::IsUnique();
			sc.type = T::GetTypeId();
			sc.data = (ui8 *)&component;
			sc.id = {};
			ComponentChanged(entityID, sc);
		}

        template <typename T, typename = std::enable_if_t<T::IsUnique() == false>> void ComponentChanged(EntityID entityID, const T &component, ComponentID id)
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

        template <typename T, typename = std::enable_if_t<T::IsUnique()>> void ComponentRemoved(EntityID entityID, const T &)
        {
            ComponentRemoved(entityID, T::GetTypeId(), {});
        }

        template <typename T, typename = std::enable_if_t<T::IsUnique() == false>> void ComponentRemoved(EntityID entityID, const T &, ComponentID id)
        {
            ComponentRemoved(entityID, T::GetTypeId(), id);
        }
        
        ComponentArrayBuilder &AddEntity(EntityID entityID); // archetype will be computed after all the components were added, you can ignore the returned value if you don't want to add any components
        void ComponentAdded(EntityID entityID, const SerializedComponent &sc);
        void ComponentChanged(EntityID entityID, const SerializedComponent &sc);
        void ComponentRemoved(EntityID entityID, StableTypeId type, ComponentID componentID);
        void RemoveEntity(EntityID entityID);
        void RemoveEntity(EntityID entityID, Archetype archetype);
    
	private:
		ComponentArrayBuilder _cab{};
		MessageStreamsBuilderEntityAdded _entityAddedStreams{};
        MessageStreamsBuilderComponentAdded _componentAddedStreams{};
        MessageStreamsBuilderComponentChanged _componentChangedStreams{};
        MessageStreamsBuilderComponentRemoved _componentRemovedStreams{};
        MessageStreamsBuilderEntityRemoved _entityRemovedStreams{};
        vector<EntityID> _entityRemovedNoArchetype{};
		EntityID _currentEntityId{};
        string _sourceName{};
	};
}