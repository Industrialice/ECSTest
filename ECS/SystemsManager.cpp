#include "PreHeader.hpp"
#include "SystemsManager.hpp"

using namespace ECSTest;

//void SystemsManager::RemoveListener(ListenerLocation *instance, void *handle)
//{
//	instance->_manager->RemoveListener(*(ListenerHandle *)handle);
//}
//
//auto SystemsManager::OnSystemExecuted(TypeId systemType, OnSystemExecutedCallbackType callback) -> ListenerHandle
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
//void SystemsManager::Register(System &system, optional<ui32> stepMicroSeconds, const vector<TypeId> &runBefore, const vector<TypeId> &runAfter, std::thread::id affinityThread)
//{
//}

auto SystemsManager::CreatePipelineGroup(optional<ui32> stepMicroSeconds, bool isMergeWithExisting) -> PipelineGroup
{
    PipelineGroup group;
    if (isMergeWithExisting)
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
    _pipelines.push_back({stepMicroSeconds});
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

	for (auto &[step, systems, thread] : _pipelines)
	{
		for (const auto &existingSystem : systems)
		{
			if (existingSystem->Type() == system->Type())
			{
				SOFTBREAK; // system with such type already exists
				return;
			}
		}
	}

    _pipelines[pipelineGroup.index].systems.push_back(move(system));
}

void SystemsManager::Unregister(TypeId systemType)
{
    if (_entitiesLocations.size())
    {
        HARDBREAK; // unregistering of the systems with non-empty scene is currently unsupported
        return;
    }

    for (auto &[step, systems, thread] : _pipelines)
    {
        for (auto &system : systems)
        {
            if (system->Type() == systemType)
            {
                auto diff = &system - &systems.front();
                systems.erase(systems.begin() + diff);
                return;
            }
        }
    }

    SOFTBREAK; // system not found
}

static void ComputeComponentIndexes(vector<ui16> &assignedIndexes, const Array<EntitiesStream::ComponentDesc> components)
{
    assignedIndexes.resize(components.size());

    if (components.size() == 0)
    {
        return;
    }

    for (uiw outer = 0; outer < components.size() - 1; ++outer)
    {
        ui16 index = 0;
        for (uiw inner = outer + 1; inner < components.size(); ++inner)
        {
            if (components[outer].type == components[inner].type)
            {
                ++index;
            }
        }

        assignedIndexes[outer] = index;
    }

    // can skip this, it's already 0
    //assignedIndexes.back() = 0;
}

static Archetype ComputeArchetype(const vector<ui16> &assignedIndexes, const Array<EntitiesStream::ComponentDesc> components)
{
    Archetype archetype;
    for (uiw index = 0; index < components.size(); ++index)
    {
        archetype.Add(components[index].type, assignedIndexes[index]);
    }
    return archetype;
}

void SystemsManager::Start(vector<std::thread> &&threads, Array<EntitiesStream> streams)
{
    ASSUME(_entitiesLocations.empty());

    _workerThreads = {threads.size()};
    for (uiw index = 0, size = threads.size(); index < size; ++index)
    {
        _workerThreads[index]._thread = move(threads[index]);
    }

    vector<ui16> assignedIndexes;

    for (auto &stream : streams)
    {
        while (auto entity = stream.Next())
        {
            ComputeComponentIndexes(assignedIndexes, entity->components);
            Archetype entityArchetype = ComputeArchetype(assignedIndexes, entity->components);
            ArchetypeGroup &archetypeGroup = FindArchetypeGroup(entityArchetype, assignedIndexes, entity->components);
            AddEntityToArchetypeGroup(archetypeGroup, *entity);
        }
    }
}

void SystemsManager::StreamIn(Array<EntitiesStream> streams)
{
    NOIMPL;
}

auto SystemsManager::FindArchetypeGroup(Archetype archetype, const vector<ui16> &assignedIndexes, const Array<EntitiesStream::ComponentDesc> components) -> ArchetypeGroup &
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

    group.uniqueTypedComponentsCount = (ui16)std::count(assignedIndexes.begin(), assignedIndexes.end(), 0);
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
        componentArray.excludes = component.excludes; // TODO: check excludes
        ++componentArray.stride;
    }

    group.reservedCount = 8; // by default initialize enough space for 8 entities

    for (ui16 index = 0; index < group.uniqueTypedComponentsCount; ++index)
    {
        auto &componentArray = group.components[index];

        // allocate memory
		ASSUME(componentArray.sizeOf > 0 && componentArray.stride > 0 && group.reservedCount > 0 && componentArray.alignmentOf > 0);
        componentArray.data.reset((ui8 *)_aligned_malloc(componentArray.sizeOf * componentArray.stride * group.reservedCount, componentArray.alignmentOf));

        // and also register component type locations
        ComponentLocation location = {&group, index};
        _archetypeGroupsComponents.emplace(componentArray.type, location);
    }
    
    return group;
}

void SystemsManager::AddEntityToArchetypeGroup(ArchetypeGroup &group, const EntitiesStream::StreamedEntity &entity)
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
        }
    }

    for (const auto &component : entity.components)
    {
        auto pred = [&component](const ArchetypeGroup::ComponentArray &stored)
        {
            return stored.type == component.type;
        };
        auto &componentArray = *std::find_if(group.components.get(), group.components.get() + group.uniqueTypedComponentsCount, pred);

        memcpy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * group.entitiesCount, component.data, componentArray.sizeOf);
    }

    ++group.entitiesCount;
}
