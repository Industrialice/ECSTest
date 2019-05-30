#include "PreHeader.hpp"
#include "MessageBuilder.hpp"

using namespace ECSTest;

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
	if (!_currentEntityId)
	{
		return;
	}
	
	MessageStreamEntityAdded::EntityWithComponents entry;
    Archetype archetype = Archetype::Create<SerializedComponent, ComponentDescription, &SerializedComponent::type>(ToArray(_cab._components));
	entry.entityID = _currentEntityId;
	entry.components = move(_cab._components);
	entry.componentsData = move(_cab._data);

    const auto &target = [this](const Archetype &archetype) -> const shared_ptr<vector<MessageStreamEntityAdded::EntityWithComponents>> &
    {
        for (const auto &[key, value] : _entityAddedStreams._data)
        {
            if (key == archetype)
            {
                return value;
            }
        }
        _entityAddedStreams._data.emplace_back(archetype, make_shared<vector<MessageStreamEntityAdded::EntityWithComponents>>());
        return _entityAddedStreams._data.back().second;
    } (archetype);

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
    ASSUME(entityID);
    Flush();
    _currentEntityId = entityID;
    _cab.Clear();
    return _cab;
}

void MessageBuilder::AddComponent(EntityID entityID, const SerializedComponent &sc)
{
    ASSUME(sc.isUnique != sc.id.IsValid());

    const auto &entry = [this](TypeId type) -> const shared_ptr<vector<MessageStreamComponentAdded::EntityWithComponents>> &
    {
        for (const auto &[key, value] : _componentAddedStreams._data)
        {
            if (key == type)
            {
                return value;
            }
        }
        _componentAddedStreams._data.emplace_back(type, make_shared<vector<MessageStreamComponentAdded::EntityWithComponents>>());
        return _componentAddedStreams._data.back().second;
    } (sc.type);

    entry->emplace_back();
    auto &last = entry->back();
    last.entityID = entityID;
    last.addedComponentID = sc.id;
    last.cab.AddComponent(sc);

    ASSUME(last.components.empty() && last.componentsData.empty());
}

void MessageBuilder::ComponentChanged(EntityID entityID, const SerializedComponent &sc)
{
    ASSUME(sc.isUnique != sc.id.IsValid());
    ASSUME(sc.isTag == false);

    const auto &entry = [this](const SerializedComponent &sc) -> const shared_ptr<MessageStreamComponentChanged::InfoWithData> &
    {
        for (const auto &[key, value] : _componentChangedStreams._data)
        {
            if (key == sc.type)
            {
                return value.second;
            }
        }
        _componentChangedStreams._data.emplace_back(sc.type, pair<ComponentDescription, shared_ptr<MessageStreamComponentChanged::InfoWithData>>(sc, make_shared<MessageStreamComponentChanged::InfoWithData>()));
        return _componentChangedStreams._data.back().second.second;
    } (sc);

    uiw copyIndex = sc.sizeOf * entry->entityIds.size();

    if (copyIndex + sc.sizeOf > entry->dataReserved)
    {
        entry->dataReserved *= 2;
        entry->dataReserved += sc.sizeOf;

        ASSUME(entry->dataReserved >= copyIndex + sc.sizeOf);

		byte *oldPtr = entry->data.release();
		byte *newPtr = Allocator::MallocAlignedRuntime::Reallocate(oldPtr, entry->dataReserved, sc.alignmentOf);
        entry->data.reset(newPtr);
    }

    entry->entityIds.emplace_back(entityID);
	if (sc.isUnique == false)
	{
		entry->componentIds.emplace_back(sc.id);
	}
    MemOps::Copy(entry->data.get() + copyIndex, sc.data, sc.sizeOf);
}

void MessageBuilder::ComponentChangedHint(const ComponentDescription &desc, uiw count)
{
	const auto &entry = [this](const ComponentDescription &desc) -> const shared_ptr<MessageStreamComponentChanged::InfoWithData> &
	{
		for (const auto &[key, value] : _componentChangedStreams._data)
		{
			if (key == desc.type)
			{
				return value.second;
			}
		}
		_componentChangedStreams._data.emplace_back(desc.type, pair<ComponentDescription, shared_ptr<MessageStreamComponentChanged::InfoWithData>>(desc, make_shared<MessageStreamComponentChanged::InfoWithData>()));
		return _componentChangedStreams._data.back().second.second;
	} (desc);

	entry->entityIds.reserve(count);
	if (desc.isUnique == false)
	{
		entry->componentIds.reserve(count);
	}

	uiw memSize = count * desc.sizeOf;

	if (memSize > entry->dataReserved)
	{
		entry->dataReserved = (ui32)memSize;

		byte *oldPtr = entry->data.release();
		byte *newPtr = Allocator::MallocAlignedRuntime::Reallocate(oldPtr, entry->dataReserved, desc.alignmentOf);
		entry->data.reset(newPtr);
	}
}

void MessageBuilder::RemoveComponent(EntityID entityID, TypeId type, ComponentID componentID)
{
    const auto &entry = [this](TypeId type) -> const shared_ptr<MessageStreamComponentRemoved::ComponentsInfo> &
    {
        for (const auto &[key, value] : _componentRemovedStreams._data)
        {
            if (key == type)
            {
                return value;
            }
        }
        _componentRemovedStreams._data.emplace_back(type, make_shared<MessageStreamComponentRemoved::ComponentsInfo>());
        return _componentRemovedStreams._data.back().second;
    } (type);

    entry->entityIds.emplace_back(entityID);
	if (componentID)
	{
		entry->componentIds.emplace_back(componentID);
	}
}

void MessageBuilder::RemoveEntity(EntityID entityID)
{
    ASSUME(entityID);
    _entityRemovedNoArchetype.push_back(entityID);
}

void MessageBuilder::RemoveEntity(EntityID entityID, Archetype archetype)
{
	ASSUME(entityID);

    const auto &entry = [this](const Archetype &archetype) -> const shared_ptr<vector<EntityID>> &
    {
        for (const auto &[key, value] : _entityRemovedStreams._data)
        {
            if (key == archetype)
            {
                return value;
            }
        }
        _entityRemovedStreams._data.emplace_back(archetype, make_shared<vector<EntityID>>());
        return _entityRemovedStreams._data.back().second;
    } (archetype);

    entry->push_back(entityID);
}