#include "PreHeader.hpp"
#include "MessageBuilder.hpp"

using namespace ECSTest;

void MessageBuilder::ComponentArrayBuilder::Clear()
{
	_components = {};
	_data = {};
}

auto MessageBuilder::ComponentArrayBuilder::AddComponent(const EntitiesStream::ComponentDesc &desc, ComponentID id) -> ComponentArrayBuilder &
{
    SerializedComponent serialized;
    serialized.alignmentOf = desc.alignmentOf;
    serialized.data = desc.data;
    serialized.id = id;
    serialized.isUnique = desc.isUnique;
    serialized.sizeOf = desc.sizeOf;
    serialized.type = desc.type;
    return AddComponent(serialized);
}

auto MessageBuilder::ComponentArrayBuilder::AddComponent(const SerializedComponent &sc) -> ComponentArrayBuilder &
{
	ui8 *oldPtr = _data.data();
	_data.resize(_data.size() + _data.size() % sc.alignmentOf);
	uiw copyIndex = _data.size();
	_data.resize(_data.size() + sc.sizeOf);
	ui8 *newPtr = _data.data();
	if (oldPtr != newPtr)
	{
		for (auto &stored : _components)
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
	_components.push_back(sc);
	SerializedComponent &added = _components.back();
	std::copy(sc.data, sc.data + sc.sizeOf, _data.begin() + copyIndex);

    added.data = _data.data() + copyIndex;

	return *this;
}

bool MessageBuilder::IsEmpty() const
{
    return _cab._components.empty() && _entityAddedStreams._data.empty() && _entityRemovedStreams._data.empty() && _componentChangedStreams._data.empty();
}

void MessageBuilder::Clear()
{
    _cab.Clear();
    _entityAddedStreams._data.clear();
    _entityRemovedStreams._data.clear();
    _componentChangedStreams._data.clear();
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

MessageStreamsBuilderEntityRemoved &MessageBuilder::EntityRemovedStreams()
{
	return _entityRemovedStreams;
}

auto ECSTest::MessageBuilder::EntityAdded(EntityID entityID) -> ComponentArrayBuilder &
{
    ASSUME(entityID.IsValid());
    Flush();
    _currentEntityId = entityID;
    _cab.Clear();
    return _cab;
}

void MessageBuilder::ComponentAdded(EntityID entityID, const SerializedComponent &sc)
{
    auto &entry = _componentAddedStreams._data[sc.type];
    if (!entry)
    {
        entry = make_shared<MessageStreamComponentAdded::InfoWithData>();
    }

    uiw copyIndex = sc.sizeOf * entry->infos.size();

    ui8 *oldPtr = entry->data.release();
    ui8 *newPtr = (ui8 *)_aligned_realloc(oldPtr, copyIndex + sc.sizeOf, sc.alignmentOf);
    entry->data.reset(newPtr);

    if (oldPtr != newPtr)
    {
        for (auto &stored : entry->infos)
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

    entry->infos.push_back({entityID, sc});
    SerializedComponent &added = entry->infos.back().component;
    std::copy(sc.data, sc.data + sc.sizeOf, entry->data.get() + copyIndex);

    added.data = entry->data.get() + copyIndex;
}

void MessageBuilder::ComponentChanged(EntityID entityID, const SerializedComponent &sc)
{
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

    entry->infos.push_back({entityID, sc});
    SerializedComponent &added = entry->infos.back().component;
    std::copy(sc.data, sc.data + sc.sizeOf, entry->data.get() + copyIndex);

    added.data = entry->data.get() + copyIndex;
}

void MessageBuilder::EntityRemoved(Archetype archetype, EntityID entityID)
{
	ASSUME(entityID.IsValid());
    auto &target = _entityRemovedStreams._data[archetype];
    if (!target)
    {
        target = make_shared<vector<EntityID>>();
    }
    target->push_back(entityID);
}