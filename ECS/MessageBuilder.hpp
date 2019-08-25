#pragma once

#include "EntityID.hpp"
#include "Archetype.hpp"
#include "ComponentArrayBuilder.hpp"

namespace ECSTest
{
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

			template <typename T> struct NonUniqueWrapper
			{
				static_assert(T::IsUnique() == false, "Can't be used with unique components");
				static_assert(T::IsTag() == false, "Can't be used with tags components");
				const T *component{};
				ComponentID id{};
			};

			template <typename T> struct NonUniqueRefWrapper
			{
				static_assert(T::IsUnique() == false, "Can't be used with unique components");
				static_assert(T::IsTag() == false, "Can't be used with tags components");
				const T &component{};
				ComponentID id{};
			};

            WARNING_PUSH
            WARNING_DISABLE_INCREASES_REQUIRED_ALIGNMENT

            template <typename T, typename = enable_if_t<T::IsUnique()>> const T *FindComponent() const
            {
                static_assert(T::IsTag() == false, "Passed component type is a tag component, use FindTag() instead");

                for (auto &c : components)
                {
                    if (c.type == T::GetTypeId())
                    {
                        ASSUME(Funcs::IsAligned(c.data, alignof(T)));
                        return reinterpret_cast<const T *>(c.data);
                    }
                }
                return nullptr;
            }

            template <typename T, typename = enable_if_t<T::IsUnique()>> const T &GetComponent() const
            {
                auto *c = FindComponent<T>();
                ASSUME(c);
                return *c;
            }

			template <typename T, typename = enable_if_t<T::IsUnique() == false>> NonUniqueWrapper<T> TryToGetComponentAtIndex(uiw index) const
			{
				ASSUME(index < components.size());
				const auto &c = components[index];
				if (c.type == T::GetTypeId())
				{
					ASSUME(Funcs::IsAligned(c.data, alignof(T)));
					return {reinterpret_cast<const T *>(c.data), c.id};
				}
				return {};
			}

			template <typename T, typename = enable_if_t<T::IsUnique() == false>> NonUniqueRefWrapper<T> GetComponentAtIndex(uiw index) const
			{
				auto c = TryToGetComponentAtIndex<T>();
				ASSUME(c);
				return {*c.component, c.id};
			}

            WARNING_POP

            template <typename T> bool FindTag() const
            {
                static_assert(T::IsTag(), "Passed component type is not a tag component");

                for (auto &c : components)
                {
                    if (c.type == T::GetTypeId())
                    {
                        return true;
                    }
                }
                return false;
            }

			template <typename T> bool Find() const
			{
				if constexpr (T::IsTag())
				{
					return FindTag<T>();
				}
				else
				{
					return FindComponent<T>() != nullptr;
				}
			}

        private:
            vector<unique_ptr<byte[], AlignedMallocDeleter>> componentsData;
        };

    private:
        shared_ptr<const vector<EntityWithComponents>> _source{};
		Archetype _archetype{};
        string_view _sourceName{};

        MessageStreamEntityAdded(Archetype archetype, const shared_ptr<const vector<EntityWithComponents>> &source, string_view sourceName) : _archetype(archetype), _source(source), _sourceName(sourceName)
        {
            ASSUME(_source->size());
        }

		[[nodiscard]] string_view SourceName() const
		{
			return _sourceName;
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

    class MessageStreamComponentAdded
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;
        friend class MessageBuilder;
        friend class MessageStreamsBuilderComponentAdded;

    public:
        struct EntityWithComponents
        {
            friend class SystemsManagerMT;
            friend class SystemsManagerST;
            friend class MessageBuilder;

            EntityID entityID;
            ComponentID addedComponentID;
            remove_reference_t<SerializedComponent> added;
            vector<SerializedComponent> components;

            WARNING_PUSH
            WARNING_DISABLE_INCREASES_REQUIRED_ALIGNMENT

            template <typename T> [[nodiscard]] const T *FindComponent() const
            {
                static_assert(T::IsTag() == false, "Passed component type is a tag component, use FindTag() instead");

                for (auto &c : components)
                {
                    if (c.type == T::GetTypeId())
                    {
                        ASSUME(Funcs::IsAligned(c.data, alignof(T)));
                        return reinterpret_cast<const T *>(c.data);
                    }
                }
                return nullptr;
            }

            template <typename T> [[nodiscard]] const T &GetComponent() const
            {
                auto *c = FindComponent<T>();
                ASSUME(c);
                return *c;
            }

            WARNING_POP

            template <typename T> [[nodiscard]] bool FindTag() const
            {
                static_assert(T::IsTag(), "Passed component type is not a tag component");

                for (auto &c : components)
                {
                    if (c.type == T::GetTypeId())
                    {
                        return true;
                    }
                }
                return false;
            }

			template <typename T> bool Find() const
			{
				if constexpr (T::IsTag())
				{
					return FindTag<T>();
				}
				else
				{
					return FindComponent<T>() != nullptr;
				}
			}

        private:
            vector<unique_ptr<byte[], AlignedMallocDeleter>> componentsData;
            ComponentArrayBuilder cab;
        };

    private:
        shared_ptr<const vector<EntityWithComponents>> _source{};
        TypeId _type{};
		Archetype _archetype{};
		string_view _sourceName{};

        MessageStreamComponentAdded(TypeId type, Archetype archetype, const shared_ptr<const vector<EntityWithComponents>> &source, string_view sourceName) : _type(type), _archetype(archetype), _source(source), _sourceName(sourceName)
        {
            ASSUME(_source->size());
			ASSUME(_type != TypeId{});
        }

        [[nodiscard]] string_view SourceName() const
        {
            return _sourceName;
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

        [[nodiscard]] TypeId Type() const
        {
            return _type;
        }

		[[nodiscard]] Archetype Archetype() const
		{
			return _archetype;
		}
    };

    class MessageStreamComponentChanged
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;
        friend class MessageBuilder;
        friend class MessageStreamsBuilderComponentChanged;
		friend class Enumerate;

    public:
		struct EntityWithComponent
		{
			EntityID entityID;
			const SerializedComponent &component;
		};

    private:
        struct InfoWithData
        {
			vector<EntityID> entityIds{};
			vector<ComponentID> componentIds{};
			unique_ptr<byte[], AlignedMallocDeleter> data{};
            ui32 dataReserved{};
        };

        shared_ptr<const InfoWithData> _source{};
		Archetype _archetype{};
		string_view _sourceName{};
		ComponentDescription _componentDesc{};

        MessageStreamComponentChanged(Archetype archetype, const shared_ptr<const InfoWithData> &source, ComponentDescription componentDesc, string_view sourceName) : _archetype(archetype), _source(source), _componentDesc(componentDesc), _sourceName(sourceName)
        {
            ASSUME(_source->entityIds.size());
			ASSUME(componentDesc.type != TypeId{});
        }

        [[nodiscard]] string_view SourceName() const
        {
            return _sourceName;
        }

    public:
		template <typename T> class const_iterator
		{
			const EntityID *const _start{};
			const EntityID *_entityIdPtr{};
			const ComponentID *_componentIdPtr{};
			const byte *_data{};

		public:
			const_iterator(const EntityID *entityIdStart, const ComponentID *componentIdStart, const byte *data) : _start(entityIdStart), _entityIdPtr(entityIdStart), _componentIdPtr(componentIdStart), _data(data) {}

			struct Info
			{
				const T &component;
				const EntityID entityID;
			};

			struct InfoWithId
			{
				const T &component;
				const EntityID entityID;
				const ComponentID componentID;
			};

			const_iterator &operator ++ ()
			{
				++_entityIdPtr;
				return *this;
			}

			[[nodiscard]] bool operator != (const const_iterator &other) const
			{
				return _entityIdPtr != other._entityIdPtr;
			}

			WARNING_PUSH
			WARNING_DISABLE_INCREASES_REQUIRED_ALIGNMENT

			[[nodiscard]] auto operator * () const
			{
				auto index = _entityIdPtr - _start;
				auto *dataPtr = _data + index * sizeof(T);

				ASSUME(Funcs::IsAligned(dataPtr, alignof(T)));

				if constexpr (T::IsUnique())
				{
					return Info{*reinterpret_cast<const T *>(dataPtr), *_entityIdPtr};
				}
				else
				{
					return InfoWithId{*reinterpret_cast<const T *>(dataPtr), *_entityIdPtr, _componentIdPtr[index]};
				}
			}

			WARNING_POP
		};

		template <typename T> class Enumerator
		{
			const MessageStreamComponentChanged &_source;
			bool _isEmpty{};

		public:
			Enumerator(const MessageStreamComponentChanged &source, bool isEmpty) : _source(source), _isEmpty(isEmpty)
			{
			}

			[[nodiscard]] const_iterator<T> begin() const
			{
				if (_isEmpty)
				{
					return {nullptr, nullptr, nullptr};
				}
				else
				{
					return {_source._source->entityIds.data(), _source._source->componentIds.data(), _source._source->data.get()};
				}
			}

			[[nodiscard]] const_iterator<T> end() const
			{
				if (_isEmpty)
				{
					return {nullptr, nullptr, nullptr};
				}
				else
				{
					return {_source._source->entityIds.data() + _source._source->entityIds.size(), nullptr, nullptr};
				}
			}
		};

		template <typename T> [[nodiscard]] Enumerator<T> Enumerate() const
		{
			return {*this, T::GetTypeId() != _componentDesc.type};
		}

		[[nodiscard]] TypeId Type() const
		{
			return _componentDesc.type;
		}

		[[nodiscard]] Archetype Archetype() const
		{
			return _archetype;
		}

        [[nodiscard]] const ComponentDescription &ComponentDesc() const
        {
            return _componentDesc;
        }
    };

    class MessageStreamComponentRemoved
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;
        friend class MessageBuilder;
        friend class MessageStreamsBuilderComponentRemoved;
		friend class Enumerate;

    public:
        struct ComponentsInfo
        {
            friend class MessageBuilder;

            vector<EntityID> entityIds;
            vector<ComponentID> componentIds;
        };

    private:
        shared_ptr<const ComponentsInfo> _source{};
        TypeId _type{};
		Archetype _archetype{};
		string_view _sourceName{};

        MessageStreamComponentRemoved(TypeId type, Archetype archetype, const shared_ptr<const ComponentsInfo> &source, string_view sourceName) : _type(type), _archetype(archetype), _source(source), _sourceName(sourceName)
        {
            ASSUME(_source->entityIds.size());
			ASSUME(_type != TypeId{});
        }

        [[nodiscard]] string_view SourceName() const
        {
            return _sourceName;
        }

    public:
		template <typename T> class const_iterator
		{
			const EntityID *const _start{};
			const EntityID *_entityIdPtr{};
			const ComponentID *_componentIdPtr{};

		public:
			const_iterator(const EntityID *entityIdStart, const ComponentID *componentIdStart) : _start(entityIdStart), _entityIdPtr(entityIdStart), _componentIdPtr(componentIdStart) {}

			struct Info
			{
				const EntityID entityID;
			};

			struct InfoWithId
			{
				const EntityID entityID;
				const ComponentID componentID;
			};

			const_iterator &operator ++ ()
			{
				++_entityIdPtr;
				return *this;
			}

			[[nodiscard]] bool operator != (const const_iterator &other) const
			{
				return _entityIdPtr != other._entityIdPtr;
			}

			[[nodiscard]] auto operator * () const
			{
				auto index = _entityIdPtr - _start;

				if constexpr (T::IsUnique())
				{
					return Info{*_entityIdPtr};
				}
				else
				{
					return InfoWithId{*_entityIdPtr, _componentIdPtr[index]};
				}
			}
		};

		template <typename T> class Enumerator
		{
			const MessageStreamComponentRemoved &_source;
			bool _isEmpty{};

		public:
			Enumerator(const MessageStreamComponentRemoved &source, bool isEmpty) : _source(source), _isEmpty(isEmpty)
			{
			}

			[[nodiscard]] const_iterator<T> begin() const
			{
				if (_isEmpty)
				{
					return {nullptr, nullptr};
				}
				else
				{
					return {_source._source->entityIds.data(), _source._source->componentIds.data()};
				}
			}

			[[nodiscard]] const_iterator<T> end() const
			{
				if (_isEmpty)
				{
					return {nullptr, nullptr};
				}
				else
				{
					return {_source._source->entityIds.data() + _source._source->entityIds.size(), nullptr};
				}
			}
		};

		template <typename T> [[nodiscard]] Enumerator<T> Enumerate() const
		{
			return {*this, T::GetTypeId() != _type};
		}

        [[nodiscard]] TypeId Type() const
        {
            return _type;
        }

		[[nodiscard]] Archetype Archetype() const
		{
			return _archetype;
		}
    };

	class MessageStreamEntityRemoved
	{
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;

        shared_ptr<const vector<EntityID>> _source{};
        Archetype _archetype;
        string_view _sourceName{};

		MessageStreamEntityRemoved(Archetype archetype, const shared_ptr<const vector<EntityID>> &source, string_view sourceName) : _archetype(archetype), _source(source), _sourceName(sourceName)
		{
            ASSUME(source->size());
        }

        [[nodiscard]] string_view SourceName() const
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

        vector<pair<Archetype, shared_ptr<vector<MessageStreamEntityAdded::EntityWithComponents>>>> _data{};
    };

    class MessageStreamsBuilderComponentAdded
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;
        friend UnitTests;

        vector<pair<TypeId, shared_ptr<vector<MessageStreamComponentAdded::EntityWithComponents>>>> _data{};
    };

    class MessageStreamsBuilderComponentChanged
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;
        friend UnitTests;

        vector<pair<TypeId, pair<ComponentDescription, shared_ptr<MessageStreamComponentChanged::InfoWithData>>>> _data{};
    };

    class MessageStreamsBuilderComponentRemoved
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;
        friend UnitTests;

        vector<pair<TypeId, shared_ptr<MessageStreamComponentRemoved::ComponentsInfo>>> _data{};
    };

    class MessageStreamsBuilderEntityRemoved
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
		friend class MessageBuilder;
        friend UnitTests;

		vector<pair<Archetype, shared_ptr<vector<EntityID>>>> _data{};
    };

    class MessageBuilder
    {
		friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend UnitTests;

		void SetEntityIdGenerator(EntityIDGenerator *generator);
        void SourceName(string_view name);
		[[nodiscard]] string_view SourceName() const;
		[[nodiscard]] bool IsEmpty() const;
        void Clear();
		void Flush();
		[[nodiscard]] ComponentArrayBuilder &AddEntity(EntityID id);
        [[nodiscard]] MessageStreamsBuilderEntityAdded &EntityAddedStreams();
        [[nodiscard]] MessageStreamsBuilderComponentAdded &ComponentAddedStreams();
        [[nodiscard]] MessageStreamsBuilderComponentChanged &ComponentChangedStreams();
        [[nodiscard]] MessageStreamsBuilderComponentRemoved &ComponentRemovedStreams();
        [[nodiscard]] MessageStreamsBuilderEntityRemoved &EntityRemovedStreams();
        [[nodiscard]] const vector<EntityID> &EntityRemovedNoArchetype();

    public:
        template <typename T, typename = enable_if_t<T::IsUnique()>> void AddComponent(EntityID entityID, const T &component)
        {
            SerializedComponent sc;
            sc.isUnique = true;
            sc.isTag = T::IsTag();
            sc.type = T::GetTypeId();
            if constexpr (T::IsTag() == false)
            {
                sc.alignmentOf = alignof(T);
                sc.sizeOf = sizeof(T);
                sc.data = reinterpret_cast<const byte *>(&component);
            }
            AddComponent(entityID, sc);
        }

        template <typename T, typename = enable_if_t<is_base_of_v<_BaseComponentClass, T> == false>, typename = void> void AddComponent(EntityID, const T &)
        {
            static_assert(false_v<T>, "Passed value is not a component");
        }

        template <typename T, typename = enable_if_t<T::IsUnique() == false && T::IsTag() == false>> void AddComponent(EntityID entityID, const T &component, ComponentID id = {})
        {
            SerializedComponent sc;
            sc.alignmentOf = alignof(T);
            sc.sizeOf = sizeof(T);
            sc.isUnique = false;
            sc.isTag = false;
            sc.type = T::GetTypeId();
			sc.data = reinterpret_cast<const byte *>(&component);
            sc.id = id;
            AddComponent(entityID, sc);
        }

        template <typename T, typename = enable_if_t<is_base_of_v<_BaseComponentClass, T> == false>, typename = void> void AddComponent(EntityID, const T &, ComponentID = {})
        {
            static_assert(false_v<T>, "Passed value is not a component");
        }

		template <typename T, typename = enable_if_t<T::IsUnique() && T::IsTag() == false>> void ComponentChanged(EntityID entityID, const T &component)
		{
            static_assert(T::IsTag() == false, "Tag components cannot be changed");

			SerializedComponent sc;
			sc.alignmentOf = alignof(T);
			sc.sizeOf = sizeof(T);
			sc.isUnique = true;
            sc.isTag = false;
			sc.type = T::GetTypeId();
			sc.data = reinterpret_cast<const byte *>(&component);
			ComponentChanged(entityID, sc);
		}

        template <typename T, typename = enable_if_t<is_base_of_v<_BaseComponentClass, T> == false>, typename = void> void ComponentChanged(EntityID, const T &)
        {
            static_assert(false_v<T>, "Passed value is not a component");
        }

        template <typename T, typename = enable_if_t<T::IsUnique() == false && T::IsTag() == false>> void ComponentChanged(EntityID entityID, const T &component, ComponentID id)
        {
            SerializedComponent sc;
            sc.alignmentOf = alignof(T);
            sc.sizeOf = sizeof(T);
            sc.isUnique = false;
            sc.isTag = false;
            sc.type = T::GetTypeId();
			sc.data = reinterpret_cast<const byte *>(&component);
            sc.id = id;
            ComponentChanged(entityID, sc);
        }

        template <typename T, typename = enable_if_t<is_base_of_v<_BaseComponentClass, T> == false>, typename = void> void ComponentChanged(EntityID, const T &, ComponentID)
        {
            static_assert(false_v<T>, "Passed value is not a component");
        }

        template <typename T, typename = enable_if_t<T::IsUnique()>> void RemoveComponent(EntityID entityID, const T &) // both for regular unique and tag components
        {
            RemoveComponent(entityID, T::GetTypeId(), {});
        }

        template <typename T, typename = enable_if_t<is_base_of_v<_BaseComponentClass, T> == false>, typename = void> void RemoveComponent(EntityID, const T &)
        {
            static_assert(false_v<T>, "Passed value is not a component");
        }

        template <typename T, typename = enable_if_t<T::IsUnique() == false && T::IsTag() == false>> void RemoveComponent(EntityID entityID, const T &, ComponentID id)
        {
            RemoveComponent(entityID, T::GetTypeId(), id);
        }

        template <typename T, typename = enable_if_t<is_base_of_v<_BaseComponentClass, T> == false>, typename = void> void RemoveComponent(EntityID, const T &, ComponentID)
        {
            static_assert(false_v<T>, "Passed value is not a component");
        }
        
		EntityID AddEntity(string_view debugName = ""); // archetype will be computed after all the components were added, you can ignore the returned value if you don't want to add any components
        void AddComponent(EntityID entityID, const SerializedComponent &sc);
        void ComponentChanged(EntityID entityID, const SerializedComponent &sc);
		void ComponentChangedHint(const ComponentDescription &desc, uiw count);
        void RemoveComponent(EntityID entityID, TypeId type, ComponentID componentID);
        void RemoveEntity(EntityID entityID);
        void RemoveEntity(EntityID entityID, Archetype archetype);
    
	private:
		EntityIDGenerator *_entityIdGenerator{};
		ComponentArrayBuilder _cab{};
		MessageStreamsBuilderEntityAdded _entityAddedStreams{};
        MessageStreamsBuilderComponentAdded _componentAddedStreams{};
        MessageStreamsBuilderComponentChanged _componentChangedStreams{};
        MessageStreamsBuilderComponentRemoved _componentRemovedStreams{};
        MessageStreamsBuilderEntityRemoved _entityRemovedStreams{};
        vector<EntityID> _entityRemovedNoArchetype{};
		EntityID _currentEntityId{};
        string_view _sourceName{};
	};
}