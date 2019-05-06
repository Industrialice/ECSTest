#include "PreHeader.hpp"
#include "MessageBuilder.hpp"

using namespace ECSTest;

void MessageBuilder::ComponentArrayBuilder::Clear()
{
	_components = {};
	_data = {};
}

auto MessageBuilder::ComponentArrayBuilder::AddComponent(const IEntitiesStream::ComponentDesc &desc, ComponentID id) -> ComponentArrayBuilder &
{
    SerializedComponent serialized;
    serialized.alignmentOf = desc.alignmentOf;
    serialized.data = desc.data;
    serialized.id = id;
    serialized.isUnique = desc.isUnique;
    serialized.isTag = desc.isTag;
    serialized.sizeOf = desc.sizeOf;
    serialized.type = desc.type;
    return AddComponent(serialized);
}

auto MessageBuilder::ComponentArrayBuilder::AddComponent(const SerializedComponent &sc) -> ComponentArrayBuilder &
{
    uiw copyIndex;

    if (sc.isTag == false)
    {
        ui8 *oldPtr = _data.data();
        _data.resize(_data.size() + _data.size() % sc.alignmentOf);
        copyIndex = _data.size();
        _data.resize(_data.size() + sc.sizeOf);
        ui8 *newPtr = _data.data();
        if (oldPtr != newPtr)
        {
            for (auto &stored : _components)
            {
                if (stored.data)
                {
                    if (newPtr > oldPtr)
                    {
                        stored.data += newPtr - oldPtr;
                    }
                    else
                    {
                        stored.data -= oldPtr - newPtr;
                    }
                }
            }
        }
    }
    else
    {
        ASSUME(sc.data == nullptr);
    }

	_components.push_back(sc);

    if (sc.isTag == false)
    {
        SerializedComponent &added = _components.back();
        std::copy(sc.data, sc.data + sc.sizeOf, _data.begin() + copyIndex);

        added.data = _data.data() + copyIndex;
    }

	return *this;
}

void MessageBuilder::SourceName(string_view name)
{
    _sourceName = name;
}

string_view MessageBuilder::SourceName() const
{
    return _sourceName;
}

bool MessageBuilder::IsEmpty() const
{
    return 
        _cab._components.empty() && 
        _entityAddedStreams._data.empty() && 
        _entityRemovedStreams._data.empty() && 
        _componentChangedStreams._data.empty() &&
        _componentAddedStreams._data.empty() &&
        _componentRemovedStreams._data.empty() &&
        _entityRemovedNoArchetype.empty();
}

void MessageBuilder::Clear()
{
    _cab.Clear();
    _entityAddedStreams._data.clear();
    _entityRemovedStreams._data.clear();
    _componentAddedStreams._data.clear();
    _componentChangedStreams._data.clear();
    _componentRemovedStreams._data.clear();
    _entityRemovedNoArchetype.clear();
}

void MessageBuilder::Flush()
{
	if (!_currentEntityId.IsValid())
	{
		return;
	}

	MessageStreamEntityAdded::EntityWithComponents entry;
    Archetype archetype = Archetype::Create<SerializedComponent, &SerializedComponent::type>(ToArray(_cab._components));
	entry.entityID = _currentEntityId;
	entry.components = move(_cab._components);
	entry.componentsData = move(_cab._data);
    auto &target = _entityAddedStreams._data[archetype];
    if (!target)
    {
        target = make_shared<vector<MessageStreamEntityAdded::EntityWithComponents>>();
    }
	target->push_back(move(entry));

	_currentEntityId = {};
}

MessageStreamsBuilderEntityAdded &MessageBuilder::EntityAddedStreams()
{
    Flush();
    return _entityAddedStreams;
}

MessageStreamsBuilderComponentAdded &MessageBuilder::ComponentAddedStreams()
{
    return _componentAddedStreams;
}

MessageStreamsBuilderComponentChanged &MessageBuilder::ComponentChangedStreams()
{
    return _componentChangedStreams;
}

MessageStreamsBuilderComponentRemoved &MessageBuilder::ComponentRemovedStreams()
{
    return _componentRemovedStreams;
}

MessageStreamsBuilderEntityRemoved &MessageBuilder::EntityRemovedStreams()
{
	return _entityRemovedStreams;
}

const vector<EntityID> &MessageBuilder::EntityRemovedNoArchetype()
{
    return _entityRemovedNoArchetype;
}

auto ECSTest::MessageBuilder::AddEntity(EntityID entityID) -> ComponentArrayBuilder &
{
    ASSUME(entityID.IsValid());
    Flush();
    _currentEntityId = entityID;
    _cab.Clear();
    return _cab;
}

void MessageBuilder::AddComponent(EntityID entityID, const SerializedComponent &sc)
{
    ASSUME(sc.isUnique != sc.id.IsValid());

    auto &entry = _componentAddedStreams._data[sc.type];
    if (!entry)
    {
        entry = make_shared<MessageStreamComponentAdded::InfoWithData>();
    }

    uiw copyIndex;

    if (sc.isTag == false)
    {
        copyIndex = sc.sizeOf * entry->infos.size();

        ui8 *oldPtr = entry->data.release();
        ui8 *newPtr = (ui8 *)_aligned_realloc(oldPtr, copyIndex + sc.sizeOf, sc.alignmentOf);
        entry->data.reset(newPtr);

        if (oldPtr != newPtr)
        {
            for (auto &stored : entry->infos)
            {
                if (stored.component.data)
                {
                    if (newPtr > oldPtr)
                    {
                        stored.component.data += newPtr - oldPtr;
                    }
                    else
                    {
                        stored.component.data -= oldPtr - newPtr;
                    }
                }
            }
        }
    }
    else
    {
        ASSUME(sc.data == nullptr);
    }

    entry->infos.push_back({entityID, sc});

    if (sc.isTag == false)
    {
        SerializedComponent &added = entry->infos.back().component;
        std::copy(sc.data, sc.data + sc.sizeOf, entry->data.get() + copyIndex);

        added.data = entry->data.get() + copyIndex;
    }
}

void MessageBuilder::ComponentChanged(EntityID entityID, const SerializedComponent &sc)
{
    ASSUME(sc.isUnique != sc.id.IsValid());
    ASSUME(sc.isTag == false);

    auto &entry = _componentChangedStreams._data[sc.type];
    if (!entry)
    {
        entry = make_shared<MessageStreamComponentChanged::InfoWithData>();
    }

    uiw copyIndex = sc.sizeOf * entry->infos.size();

    ui8 *oldPtr = entry->data.release();
    ui8 *newPtr = (ui8 *)_aligned_realloc(oldPtr, copyIndex + sc.sizeOf, sc.alignmentOf);
    entry->data.reset(newPtr);

    if (oldPtr != newPtr)
    {
        for (auto &stored : entry->infos)
        {
            if (stored.component.data)
            {
                if (newPtr > oldPtr)
                {
                    stored.component.data += newPtr - oldPtr;
                }
                else
                {
                    stored.component.data -= oldPtr - newPtr;
                }
            }
        }
    }

    entry->infos.push_back({entityID, sc});
    SerializedComponent &added = entry->infos.back().component;
    std::copy(sc.data, sc.data + sc.sizeOf, entry->data.get() + copyIndex);

    added.data = entry->data.get() + copyIndex;
}

void MessageBuilder::RemoveComponent(EntityID entityID, StableTypeId type, ComponentID componentID)
{
    auto &entry = _componentRemovedStreams._data[type];
    if (!entry)
    {
        entry = make_shared<vector<MessageStreamComponentRemoved::ComponentInfo>>();
    }

    entry->push_back({entityID, componentID});
}

void MessageBuilder::RemoveEntity(EntityID entityID)
{
    ASSUME(entityID.IsValid());
    _entityRemovedNoArchetype.push_back(entityID);
}

void MessageBuilder::RemoveEntity(EntityID entityID, Archetype archetype)
{
	ASSUME(entityID.IsValid());
    auto &target = _entityRemovedStreams._data[archetype];
    if (!target)
    {
        target = make_shared<vector<EntityID>>();
    }
    target->push_back(entityID);
}