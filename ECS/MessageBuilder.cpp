#include "PreHeader.hpp"
#include "MessageBuilder.hpp"

using namespace ECSTest;

void MessageStreamEntityRemoved::Rewind()
{
	_current = 0;
}

optional<EntityID> MessageStreamEntityRemoved::Next()
{
	if (_current < _source.size())
	{
		return _source[_current++];
	}
	return {};
}

void MessageStreamEntityAdded::Rewind()
{
	_current = 0;
}

auto ECSTest::MessageStreamEntityAdded::Next() -> optional<EntityAndComponents>
{
	if (_current < _source.size())
	{
		uiw index = _current++;
		EntityAndComponents eac = {_source[index].entityID, ToArray(_source[index].components)};
		return eac;
	}
	return {};
}

void MessageBuilder::ComponentArrayBuilder::Clear()
{
	_archetype = {};
	_components = {};
	_data = {};
}

auto MessageBuilder::ComponentArrayBuilder::AddComponent(const EntitiesStream::ComponentDesc &desc, ui32 id) -> ComponentArrayBuilder &
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
	_data.resize(_data.size() % sc.alignmentOf);
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

	_archetype.Add(sc.type);

	return *this;
}

void MessageBuilder::Flush()
{
	if (!_currentEntityId.IsValid())
	{
		return;
	}

	MessageStreamEntityAdded::EntityWithComponents entry;
	entry.entityID = _currentEntityId;
	entry.components = move(_cab._components);
	entry.componentsData = move(_cab._data);
	_entityAddedStream._data[_cab._archetype].push_back(move(entry));

	_currentEntityId = {};
}

MessageStreamsBuilderEntityRemoved &MessageBuilder::EntityRemovedStream()
{
	return _entityRemovedStream;
}

MessageStreamsBuilderEntityAdded &MessageBuilder::EntityAddedStream()
{
	Flush();
	return _entityAddedStream;
}

void MessageBuilder::EntityRemoved(Archetype archetype, EntityID entityID)
{
	ASSUME(entityID.IsValid());
	_entityRemovedStream._data[archetype].push_back(entityID);
}

auto ECSTest::MessageBuilder::EntityAdded(EntityID entityID) -> ComponentArrayBuilder &
{
	ASSUME(entityID.IsValid());
	Flush();
	_currentEntityId = entityID;
	_cab.Clear();
	return _cab;
}