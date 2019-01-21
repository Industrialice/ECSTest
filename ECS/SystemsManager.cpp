#include "PreHeader.hpp"
#include "SystemsManager.hpp"
#include <set>

using namespace ECSTest;

//void SystemsManager::RemoveListener(ListenerLocation *instance, void *handle)
//{
//	instance->_manager->RemoveListener(*(ListenerHandle *)handle);
//}
//
//auto SystemsManager::OnSystemExecuted(StableTypeId systemType, OnSystemExecutedCallbackType callback) -> ListenerHandle
//{
//	auto id = _onSystemExecutedCurrentId;
//	++_onSystemExecutedCurrentId;
//	ASSUME(_onSystemExecutedCurrentId < 1'000'000); // it's won't break anything by itself, but indicates that you have a bug that causes callback attachment spam
//	
//	for (auto &[step, group] : _systemGroups)
//	{
//		for (auto &system : group._systems)
//		{
//			if (system._system->Type() == systemType)
//			{
//				system._onExecutedCallbacks.push_back({callback, id});
//				return {_listenerLocation, id};
//			}
//		}
//	}
//	
//	_pendingOnSystemExecutedDatas.push_back({{callback, id}, systemType});
//	return {_listenerLocation, id};
//}
//
//void SystemsManager::RemoveListener(ListenerHandle &listener)
//{
//	for (auto &[step, group] : _systemGroups)
//	{
//		for (auto &system : group._systems)
//		{
//			auto pred = [&listener](const OnSystemExecutedData &data)
//			{
//				return listener.Id() == data._id;
//			};
//			auto it = std::find_if(system._onExecutedCallbacks.begin(), system._onExecutedCallbacks.end(), pred);
//			if (it != system._onExecutedCallbacks.end())
//			{
//				system._onExecutedCallbacks.erase(it);
//				return;
//			}
//		}
//	}
//
//	auto pred = [&listener](const PendingOnSystemExecutedData &data)
//	{
//		return listener.Id() == data._data._id;
//	};
//	auto it = std::find_if(_pendingOnSystemExecutedDatas.begin(), _pendingOnSystemExecutedDatas.end(), pred);
//	if (it != _pendingOnSystemExecutedDatas.end())
//	{
//		_pendingOnSystemExecutedDatas.erase(it);
//		return;
//	}
//
//	HARDBREAK;
//}
//
//void SystemsManager::Register(System &system, optional<ui32> stepMicroSeconds, const vector<StableTypeId> &runBefore, const vector<StableTypeId> &runAfter, std::thread::id affinityThread)
//{
//}

auto SystemsManager::CreatePipelineGroup(optional<ui32> stepMicroSeconds, bool isMergeIfSuchPipelineExists) -> PipelineGroup
{
    PipelineGroup group;
    if (isMergeIfSuchPipelineExists)
    {
        for (ui32 index = 0; index < (ui32)_pipelines.size(); ++index)
        {
            if (_pipelines[index].stepMicroSeconds == stepMicroSeconds)
            {
                group.index = index;
                return group;
            }
        }
    }
    group.index = (ui32)_pipelines.size();
	_pipelines.push_back({{}, {}, stepMicroSeconds});
    return group;
}

void SystemsManager::Register(unique_ptr<System> system, PipelineGroup pipelineGroup)
{
    if (_entitiesLocations.size())
    {
        HARDBREAK; // registering of the systems with non-empty scene is currently unsupported
        return;
    }

    ASSUME(pipelineGroup.index < _pipelines.size());

	for (auto &[executionFrame, systems, step, thread] : _pipelines)
	{
		for (const auto &existingSystem : systems)
		{
			if (existingSystem.system->Type() == system->Type())
			{
				SOFTBREAK; // system with such type already exists
				return;
			}
		}
	}

	_pipelines[pipelineGroup.index].systems.push_back({move(system), 0});
}

void SystemsManager::Unregister(StableTypeId systemType)
{
    if (_entitiesLocations.size())
    {
        HARDBREAK; // unregistering of the systems with non-empty scene is currently unsupported
        return;
    }

    for (auto &[executionFrame, systems, step, thread] : _pipelines)
    {
        for (auto &system : systems)
        {
            if (system.system->Type() == systemType)
            {
                auto diff = &system - &systems.front();
                systems.erase(systems.begin() + diff);
                return;
            }
        }
    }

    SOFTBREAK; // system not found
}

static ArchetypeFull ComputeArchetype(const vector<ui32> &assignedIDs, const Array<EntitiesStream::ComponentDesc> components)
{
    ArchetypeFull archetype;
    for (uiw index = 0; index < components.size(); ++index)
    {
        archetype.Add(components[index].type, assignedIDs[index]);
    }
    return archetype;
}

void SystemsManager::AssignComponentIDs(vector<ui32> &assignedIDs, const Array<EntitiesStream::ComponentDesc> components)
{
    assignedIDs.resize(components.size());
    for (uiw index = 0; index < components.size(); ++index)
    {
        if (components[index].isUnique)
        {
            assignedIDs[index] = 0;
        }
        else
        {
            assignedIDs[index] = ++_lastComponentID;
        }
    }
}

void SystemsManager::Start(vector<std::thread> &&threads, Array<EntitiesStream> streams)
{
    ASSUME(_entitiesLocations.empty());

    _workerThreads = {threads.size()};
    for (uiw index = 0, size = threads.size(); index < size; ++index)
    {
        _workerThreads[index]._thread = move(threads[index]);
    }

    MessageBuilder messageBulder;
    vector<ui32> assignedIDs;

    for (auto &stream : streams)
    {
        while (auto entity = stream.Next())
        {
            AssignComponentIDs(assignedIDs, entity->components);
            ArchetypeFull entityArchetype = ComputeArchetype(assignedIDs, entity->components);
            ArchetypeGroup &archetypeGroup = FindArchetypeGroup(entityArchetype, assignedIDs, entity->components);
            AddEntityToArchetypeGroup(entityArchetype, archetypeGroup, *entity, assignedIDs, messageBulder);
        }
    }

	auto &entityAddedStream = messageBulder.EntityAddedStream();
	for (auto &[archetype, messages] : entityAddedStream._data)
	{
		MessageStreamEntityAdded stream = {messages};
		for (auto &pipeline : _pipelines)
		{
			for (auto &system : pipeline.systems)
			{
				IndirectSystem *indirect = system.system->AsIndirectSystem();
				if (indirect && IsSystemAcceptArchetype(archetype, indirect->RequestedComponents()))
				{
					indirect->ProcessMessages(archetype, stream);
				}
			}
		}
	}
}

void SystemsManager::StreamIn(Array<EntitiesStream> streams)
{
    NOIMPL;
}

auto SystemsManager::FindArchetypeGroup(ArchetypeFull archetype, const vector<ui32> &assignedIDs, const Array<EntitiesStream::ComponentDesc> components) -> ArchetypeGroup &
{
    auto searchResult = _archetypeGroups.find(archetype);
    if (searchResult != _archetypeGroups.end())
    {
        return searchResult->second;
    }

    // such group doesn't exist yet, adding a new one

    auto [insertedWhere, insertedResult] = _archetypeGroups.try_emplace(archetype);
    ASSUME(insertedResult);
    ArchetypeGroup &group = insertedWhere->second;

    _archetypeGroupsShort.emplace(archetype.ToShort(), &group);

    vector<StableTypeId> uniqueTypes;
    for (const auto &component : components)
    {
		if (std::find(uniqueTypes.begin(), uniqueTypes.end(), component.type) == uniqueTypes.end())
		{
			uniqueTypes.push_back(component.type);
		}
    }

    group.uniqueTypedComponentsCount = (ui16)uniqueTypes.size();
    group.components = make_unique<ArchetypeGroup::ComponentArray[]>(group.uniqueTypedComponentsCount);

    for (const auto &component : components)
    {
        auto pred = [&component](const ArchetypeGroup::ComponentArray &stored)
        {
            return stored.sizeOf == 0 || stored.type == component.type;
        };
        auto &componentArray = *std::find_if(group.components.get(), group.components.get() + group.uniqueTypedComponentsCount, pred);

        componentArray.alignmentOf = component.alignmentOf;
        componentArray.sizeOf = component.sizeOf;
        componentArray.type = component.type;
        componentArray.isUnique = component.isUnique; // TODO: ensure uniquiness
        ++componentArray.stride;
    }

    group.reservedCount = 8; // by default initialize enough space for 8 entities

    for (ui16 index = 0; index < group.uniqueTypedComponentsCount; ++index)
    {
        auto &componentArray = group.components[index];

        // allocate memory
		ASSUME(componentArray.sizeOf > 0 && componentArray.stride > 0 && group.reservedCount > 0 && componentArray.alignmentOf > 0);
        componentArray.data.reset((ui8 *)_aligned_malloc(componentArray.sizeOf * componentArray.stride * group.reservedCount, componentArray.alignmentOf));

        if (!componentArray.isUnique)
        {
            componentArray.ids.reset((ui32 *)malloc(sizeof(ui32) * componentArray.stride * group.reservedCount));
        }

        // and also register component type locations
        ComponentLocation location = {&group, index};
        _archetypeGroupsComponents.emplace(componentArray.type, location);
    }

	// also add that archetype to the library
	_archetypeReflector.AddToLibrary(archetype.ToShort(), move(uniqueTypes));
    
    return group;
}

void SystemsManager::AddEntityToArchetypeGroup(ArchetypeFull archetype, ArchetypeGroup &group, const EntitiesStream::StreamedEntity &entity, const vector<ui32> &assignedIDs, MessageBuilder &messageBuilder)
{
    if (group.entitiesCount == group.reservedCount)
    {
        group.reservedCount *= 2;
        
        for (ui16 index = 0; index < group.uniqueTypedComponentsCount; ++index)
        {
            auto &componentArray = group.components[index];

			ASSUME(componentArray.sizeOf > 0 && componentArray.stride > 0 && group.reservedCount && componentArray.alignmentOf > 0);

            void *oldPtr = componentArray.data.release();
            void *newPtr = _aligned_realloc(oldPtr, componentArray.sizeOf * componentArray.stride * group.reservedCount, componentArray.alignmentOf);
            componentArray.data.reset((ui8 *)newPtr);

            if (!componentArray.isUnique)
            {
                ui32 *oldUPtr = componentArray.ids.release();
                ui32 *newUPtr = (ui32 *)realloc(oldUPtr, sizeof(ui32) * componentArray.stride * group.reservedCount);
                componentArray.ids.reset(newUPtr);
            }
        }
    }

    auto &componentBuilder = messageBuilder.EntityAdded(entity.entityId);

    for (uiw index = 0; index < entity.components.size(); ++index)
    {
        const auto &component = entity.components[index];

        auto pred = [&component](const ArchetypeGroup::ComponentArray &stored)
        {
            return stored.type == component.type;
        };
        auto &componentArray = *std::find_if(group.components.get(), group.components.get() + group.uniqueTypedComponentsCount, pred);

        memcpy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * group.entitiesCount, component.data, componentArray.sizeOf);

        ui32 componentId = 0;
        if (!componentArray.isUnique)
        {
            componentArray.ids[componentArray.stride * group.entitiesCount] = assignedIDs[index];
        }

        componentBuilder.AddComponent(component, assignedIDs[index]);
    }

    ++group.entitiesCount;
}

bool SystemsManager::IsSystemAcceptArchetype(Archetype archetype, Array<const System::RequestedComponent> systemComponents) const
{
	auto reflected = _archetypeReflector.Reflect(archetype);
	for (auto &requested : systemComponents)
	{
		if (requested.requirement == System::ComponentRequirement::Optional)
		{
			continue;
		}
		bool isFound = reflected.find(requested.type) != reflected.end();
		if (requested.requirement == System::ComponentRequirement::Subtractive)
		{
			if (isFound)
			{
				return false;
			}
		}
		else
		{
			ASSUME(requested.requirement == System::ComponentRequirement::Required);
			if (!isFound)
			{
				return false;
			}
		}
	}
	return true;
}
