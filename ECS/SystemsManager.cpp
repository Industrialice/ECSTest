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
    _pipelines.emplace_back();
    _pipelines.back().stepMicroSeconds = stepMicroSeconds;
    return group;
}

void SystemsManager::Register(unique_ptr<System> system, PipelineGroup pipelineGroup)
{
    ASSUME(system);

    if (_entitiesLocations.size())
    {
        HARDBREAK; // registering of the systems with non-empty scene is currently unsupported
        return;
    }

    ASSUME(pipelineGroup.index < _pipelines.size());

    bool isDirectSystem = system->AsDirectSystem() != nullptr;
    auto requestedComponents = system->RequestedComponents();

    if (isDirectSystem)
    {
        uiw requiredCount = requestedComponents.required.size();
        uiw optionalCount = requestedComponents.optional.size();

        if (requiredCount == 0 && optionalCount == 0)
        {
            SOFTBREAK; // the system will never be executed with such configuration
            return;
        }
    }

	for (auto &[systemsAtExecution, executionFrame, directSystems, indirectSystems, step] : _pipelines)
	{
        if (isDirectSystem)
        {
            for (const auto &existingSystem : directSystems)
            {
                if (existingSystem.system->Type() == system->Type())
                {
                    SOFTBREAK; // system with such type already exists
                    return;
                }
            }
        }
        else
        {
            for (const auto &existingSystem : indirectSystems)
            {
                if (existingSystem.system->Type() == system->Type())
                {
                    SOFTBREAK; // system with such type already exists
                    return;
                }
            }
        }
	}

    vector<pair<StableTypeId, RequirementForComponent>> types;
    types.reserve(requestedComponents.all.size());
    for (auto req : requestedComponents.all)
    {
        types.push_back({req.type, req.requirement});
    }
    _archetypeReflector.StartTrackingMatchingArchetypes((uiw)system.get(), ToArray(types));

    auto &pipeline = _pipelines[pipelineGroup.index];

    if (isDirectSystem)
    {
		ManagedDirectSystem direct;
        direct.executedAt = pipeline.executionFrame - 1;
		direct.system.reset(system.release()->AsDirectSystem());
        pipeline.directSystems.push_back(move(direct));
    }
    else
    {
		ManagedIndirectSystem indirect;
        indirect.executedAt = pipeline.executionFrame - 1;
		indirect.system.reset(system.release()->AsIndirectSystem());
        pipeline.indirectSystems.push_back(move(indirect));
    }
}

void SystemsManager::Unregister(StableTypeId systemType)
{
    if (_entitiesLocations.size())
    {
        HARDBREAK; // unregistering of the systems with non-empty scene is currently unsupported
        return;
    }

    for (auto &[systemsAtExecution, executionFrame, directSystems, indirectSystems, step] : _pipelines)
    {
        for (auto &system : directSystems)
        {
            if (system.system->Type() == systemType)
            {
                _archetypeReflector.StopTrackingMatchingArchetypes((uiw)system.system.get());

                auto diff = &system - &directSystems.front();
                directSystems.erase(directSystems.begin() + diff);
                return;
            }
        }
        for (auto &system : indirectSystems)
        {
            if (system.system->Type() == systemType)
            {
                _archetypeReflector.StopTrackingMatchingArchetypes((uiw)system.system.get());

                auto diff = &system - &indirectSystems.front();
                indirectSystems.erase(indirectSystems.begin() + diff);
                return;
            }
        }
    }

    SOFTBREAK; // system not found
}

static ArchetypeFull ComputeArchetype(const vector<ui32> &assignedIDs, Array<const EntitiesStream::ComponentDesc> components)
{
    ArchetypeFull archetype;
    for (uiw index = 0; index < components.size(); ++index)
    {
        archetype.Add(components[index].type, assignedIDs[index]);
    }
    return archetype;
}

static void StreamedToSerialized(Array<const EntitiesStream::ComponentDesc> streamed, vector<SerializedComponent> &serialized)
{
    serialized.resize(streamed.size());
    for (uiw index = 0; index < streamed.size(); ++index)
    {
        auto &s = streamed[index];
        auto &t = serialized[index];

        t.alignmentOf = s.alignmentOf;
        t.data = s.data;
        t.id = 0;
        t.isUnique = s.isUnique;
        t.sizeOf = s.sizeOf;
        t.type = s.type;
    }
}

static ArchetypeFull ComputeArchetype(Array<const SerializedComponent> components)
{
    ArchetypeFull archetype;
    for (uiw index = 0; index < components.size(); ++index)
    {
        archetype.Add(components[index].type, components[index].id);
    }
    return archetype;
}

static void AssignComponentIDs(Array<SerializedComponent> components, std::atomic<ui32> &lastComponentId)
{
    for (uiw index = 0; index < components.size(); ++index)
    {
        if (!components[index].isUnique)
        {
            components[index].id = ++lastComponentId;
        }
        else
        {
            ASSUME(components[index].id == 0);
        }
    }
}

void SystemsManager::Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, Array<shared_ptr<EntitiesStream>> streams)
{
    ASSUME(_entitiesLocations.empty());

    if (workers.empty())
    {
        SOFTBREAK;
        workers.emplace_back();
    }

    _workerThreads = {workers.size()};
    for (uiw index = 0, size = workers.size(); index < size; ++index)
    {
        Funcs::Reinitialize(_workerThreads[index], move(workers[index]));
        _workerThreads[index].SetOnWorkDoneNotifier(_workerFinishedWorkNotifier);
        if (!_workerThreads[index].IsRunning())
        {
            _workerThreads[index].Start();
        }
    }

    _idGenerator = move(idGenerator);

    _isStoppingExecution = false;

    _schedulerThread = std::thread([this, streams] { StartScheduler(streams); });
}

void SystemsManager::Stop(bool isWaitForStop)
{
    _isStoppingExecution = true;
    if (isWaitForStop)
    {
        _schedulerThread.join();
    }
	Funcs::Reinitialize(_entitiesLocations);
	Funcs::Reinitialize(_archetypeGroups);
	//Funcs::Reinitialize(_archetypeGroupsComponents);
	Funcs::Reinitialize(_archetypeGroupsFull);
	Funcs::Reinitialize(_archetypeReflector);
	_lastComponentId = 0;
	Funcs::Reinitialize(_workerThreads);
}

bool SystemsManager::IsRunning()
{
    return _schedulerThread.joinable();
}

void SystemsManager::StreamIn(Array<shared_ptr<EntitiesStream>> streams)
{
    NOIMPL;
}

auto SystemsManager::FindArchetypeGroup(ArchetypeFull archetype, Array<const SerializedComponent> components) -> ArchetypeGroup &
{
    auto searchResult = _archetypeGroupsFull.find(archetype);
    if (searchResult != _archetypeGroupsFull.end())
    {
        return searchResult->second;
    }

    // such group doesn't exist yet, adding a new one
    return AddNewArchetypeGroup(archetype, components);
}

auto SystemsManager::AddNewArchetypeGroup(ArchetypeFull archetype, Array<const SerializedComponent> components) -> ArchetypeGroup &
{
    auto[insertedWhere, insertedResult] = _archetypeGroupsFull.try_emplace(archetype);
    ASSUME(insertedResult);
    ArchetypeGroup &group = insertedWhere->second;

    _archetypeGroups[archetype.ToShort()].emplace_back(std::ref(group));

    vector<StableTypeId> uniqueTypes;
    for (const auto &component : components)
    {
        if (std::find(uniqueTypes.begin(), uniqueTypes.end(), component.type) == uniqueTypes.end())
        {
            uniqueTypes.push_back(component.type);
        }
    }

    group.archetype = archetype;

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
        //ComponentLocation location = {&group, index};
        //_archetypeGroupsComponents.emplace(componentArray.type, location);
    }

    group.entities.reset((EntityID *)malloc(sizeof(EntityID) * group.reservedCount));

    // also add that archetype to the library
    _archetypeReflector.AddToLibrary(archetype.ToShort(), move(uniqueTypes));

    return group;
}

void SystemsManager::AddEntityToArchetypeGroup(ArchetypeFull archetype, ArchetypeGroup &group, EntityID entityId, Array<const SerializedComponent> components, MessageBuilder *messageBuilder)
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

        EntityID *oldPtr = group.entities.release();
        EntityID *newPtr = (EntityID *)realloc(oldPtr, sizeof(EntityID) * group.reservedCount);
        group.entities.reset(newPtr);
    }

    optional<std::reference_wrapper<MessageBuilder::ComponentArrayBuilder>> componentBuilder;
    if (messageBuilder)
    {
        componentBuilder = messageBuilder->EntityAdded(entityId);
    }

    for (uiw index = 0; index < components.size(); ++index)
    {
        const auto &component = components[index];

        auto pred = [&component](const ArchetypeGroup::ComponentArray &stored)
        {
            return stored.type == component.type;
        };
        auto &componentArray = *std::find_if(group.components.get(), group.components.get() + group.uniqueTypedComponentsCount, pred);

        memcpy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * group.entitiesCount, component.data, componentArray.sizeOf);

        ui32 componentId = 0;
        if (!componentArray.isUnique)
        {
            componentArray.ids[componentArray.stride * group.entitiesCount] = component.id;
        }

        if (componentBuilder)
        {
            componentBuilder->get().AddComponent(component);
        }
    }

    group.entities[group.entitiesCount] = entityId;

    auto entitiesLocationsLock = _entitiesLocationsLock.Lock(DIWRSpinLock::LockType::Exclusive);
    _entitiesLocations[entityId] = {&group, group.entitiesCount};
    entitiesLocationsLock.Unlock();

    ++group.entitiesCount;
}

bool SystemsManager::IsSystemAcceptsArchetype(Archetype archetype, Array<const System::RequestedComponent> systemComponents) const
{
	auto reflected = _archetypeReflector.Reflect(archetype);
	for (auto &requested : systemComponents)
	{
		if (requested.requirement == RequirementForComponent::Optional)
		{
			continue;
		}
		bool isFound = reflected.find(requested.type) != reflected.end();
		if (requested.requirement == RequirementForComponent::Subtractive)
		{
			if (isFound)
			{
				return false;
			}
		}
		else
		{
			ASSUME(requested.requirement == RequirementForComponent::Required);
			if (!isFound)
			{
				return false;
			}
		}
	}
	return true;
}

void SystemsManager::StartScheduler(Array<shared_ptr<EntitiesStream>> streams)
{
    MessageBuilder messageBulder;
    vector<SerializedComponent> serialized;

    for (auto &stream : streams)
    {
        while (auto entity = stream->Next())
        {
            StreamedToSerialized(entity->components, serialized);
            AssignComponentIDs(ToArray(serialized), _lastComponentId);
            ArchetypeFull entityArchetype = ComputeArchetype(ToArray(serialized));
            ArchetypeGroup &archetypeGroup = FindArchetypeGroup(entityArchetype, ToArray(serialized));
            AddEntityToArchetypeGroup(entityArchetype, archetypeGroup, entity->entityId, ToArray(serialized), &messageBulder);
        }
    }

    auto &entityAddedStreams = messageBulder.EntityAddedStreams();
    for (auto &[archetype, messages] : entityAddedStreams._data)
    {
        MessageStreamEntityAdded stream = {archetype, messages};
        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (IsSystemAcceptsArchetype(archetype, managed.system->RequestedComponents().all))
                {
                    managed.messageQueue.entityAddedStreams.push_back(stream);
                }
            }
        }
    }

    uiw workerIndex = 0;
    for (auto &pipeline : _pipelines)
    {
        for (auto &managed : pipeline.indirectSystems)
        {
            if (managed.messageQueue.entityAddedStreams.size())
            {
                _workerThreads[workerIndex++].AddWork(std::bind(TaskProcessMessages, std::ref(*managed.system), std::cref(managed.messageQueue)));
                if (workerIndex == _workerThreads.size())
                {
                    workerIndex = 0;
                }
            }
        }
    }

    // wait for completion of initial entities processing
    for (auto &worker : _workerThreads)
    {
        std::unique_lock lock{_workerFinishedWorkNotifier->first};
        _workerFinishedWorkNotifier->second.wait(lock, [&worker] { return worker.WorkInProgressCount() == 0; });
    }

    for (auto &pipeline : _pipelines)
    {
        for (auto &managed : pipeline.indirectSystems)
        {
            managed.messageQueue.clear();
        }
    }

    while (!_isStoppingExecution)
    {
        SchedulerLoop();
    }

    for (auto &worker : _workerThreads)
    {
        worker.Stop();
    }

    for (auto &worker : _workerThreads)
    {
        worker.Join();
    }
}

void SystemsManager::SchedulerLoop()
{
    for (auto &pipeline : _pipelines)
    {
        ExecutePipeline(pipeline);
    }
}

void SystemsManager::ExecutePipeline(Pipeline &pipeline)
{
    if (*pipeline.systemsAtExecution) // previously submitted work is not completed yet
    {
        std::this_thread::yield();
        return;
    }

    bool isAddedWork = false;

    for (auto &managed : pipeline.directSystems)
    {
    }

    for (auto &managed : pipeline.indirectSystems)
    {
        if (managed.executedAt != pipeline.executionFrame)
        {
            managed.executedAt = pipeline.executionFrame;
            CalculateGroupsToExectute(managed.system.get(), managed.groupsToExecute);
        }

        ASSUME(managed.groupLocks.empty());
        ASSUME(managed.componentLocks.empty());

        auto requested = managed.system->RequestedComponents();

        // we only need to lock only the components that were requested for write, if there're no such components, skip the locking altogether
        if (requested.writeAccess.size())
        {
            auto tryLockComponents = [&requested, &managed]() -> bool
            {
                for (ArchetypeGroup &group : managed.groupsToExecute)
                {
                    bool hasInclusiveLocks = false;
                    auto groupLock = group.lock.TryLock(DIWRSpinLock::LockType::Read);
                    if (!groupLock)
                    {
                        return false;
                    }

                    if (requested.required.size())
                    {
                        // locate and add all required components
                        for (System::RequestedComponent request : requested.required)
                        {
                            for (uiw index = 0; ; ++index)
                            {
                                ASSUME(index < group.uniqueTypedComponentsCount);

                                if (group.components[index].type == request.type)
                                {
                                    if (request.isWriteAccess)
                                    {
                                        auto componentLock = group.components[index].lock.TryLock(DIWRSpinLock::LockType::Inclusive);
                                        if (!componentLock)
                                        {
                                            return false;
                                        }
                                        managed.componentLocks.emplace_back(move(*componentLock));
                                        hasInclusiveLocks |= true;
                                    }

                                    break;
                                }
                            }

                        }

                        // try to locate and add any optionally requested components
                        for (System::RequestedComponent opt : requested.optional)
                        {
                            for (uiw index = 0; index < group.uniqueTypedComponentsCount; ++index)
                            {
                                if (group.components[index].type == opt.type)
                                {
                                    if (opt.isWriteAccess)
                                    {
                                        auto componentLock = group.components[index].lock.TryLock(DIWRSpinLock::LockType::Inclusive);
                                        if (!componentLock)
                                        {
                                            return false;
                                        }
                                        managed.componentLocks.emplace_back(move(*componentLock));
                                        hasInclusiveLocks |= true;
                                    }

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        // the group hasn't reqested any types, add all components
                        for (uiw index = 0; index < group.uniqueTypedComponentsCount; ++index)
                        {
                            const auto &component = group.components[index];
                            bool isWriteAccess = false;

                            auto req = requested.optional.find_if([&component](const System::RequestedComponent &req) { return req.type == component.type; });
                            if (req != requested.optional.end())
                            {
                                ASSUME(req->requirement == RequirementForComponent::Optional);
                                isWriteAccess = req->isWriteAccess;
                            }

                            if (isWriteAccess)
                            {
                                auto componentLock = component.lock.TryLock(DIWRSpinLock::LockType::Inclusive);
                                if (!componentLock)
                                {
                                    return false;
                                }
                                managed.componentLocks.emplace_back(move(*componentLock));
                                hasInclusiveLocks |= true;
                            }
                        }
                    }

                    if (hasInclusiveLocks)
                    {
                        managed.groupLocks.emplace_back(pair(group.archetype, move(*groupLock)));
                    }
                    else
                    {
                        groupLock->Unlock();
                    }
                }

                return true;
            };

            if (!tryLockComponents())
            {
                for (auto &lock : managed.componentLocks)
                {
                    lock.Unlock();
                }
                managed.componentLocks.clear();

                for (auto &lock : managed.groupLocks)
                {
                    lock.second.Unlock();
                }
                managed.groupLocks.clear();

                continue;
            }
        }

        pipeline.systemsAtExecution->fetch_add(1);

        System::Environment env =
        {
            0,
            0,
            0,
            _idGenerator
        };

        auto work = std::bind(&SystemsManager::TaskExecuteIndirectSystem, this, std::ref(*managed.system), move(managed.messageQueue), env, std::ref(*pipeline.systemsAtExecution), std::ref(managed.groupLocks), std::ref(managed.componentLocks));
        FindBestWorker().AddWork(work);
        isAddedWork = true;

        managed.messageQueue = {};
    }

    if (!isAddedWork)
    {
        std::this_thread::yield();
    }
}

void SystemsManager::CalculateGroupsToExectute(const System *system, vector<std::reference_wrapper<ArchetypeGroup>> &groups)
{
    groups.clear();
    
    const auto &archetypes = _archetypeReflector.FindMatchingArchetypes((uiw)system);

    auto lock = _archetypeGroupsLock.Lock(DIWRSpinLock::LockType::Read);

    for (Archetype archetype : archetypes)
    {
        auto it = _archetypeGroups.find(archetype);
        ASSUME(it != _archetypeGroups.end());
        groups.insert(groups.end(), it->second.begin(), it->second.end());
    }

    lock.Unlock();
}

WorkerThread &SystemsManager::FindBestWorker()
{
    WorkerThread *bestWorker = _workerThreads.data();
    for (auto &worker : _workerThreads)
    {
        if (worker.WorkInProgressCount() < bestWorker->WorkInProgressCount())
        {
            bestWorker = &worker;
        }
    }
    return *bestWorker;
}

void SystemsManager::TaskProcessMessages(IndirectSystem &system, const ManagedIndirectSystem::MessageQueue &messageQueue)
{
    for (const auto &stream : messageQueue.entityAddedStreams)
    {
        system.ProcessMessages(stream);
    }
    for (const auto &stream : messageQueue.entityRemovedStreams)
    {
        system.ProcessMessages(stream);
    }
}

void SystemsManager::TaskExecuteIndirectSystem(IndirectSystem &system, ManagedIndirectSystem::MessageQueue messageQueue, System::Environment env, std::atomic<ui32> &decrementAtCompletion, vector<pair<ArchetypeFull, DIWRSpinLock::Unlocker>> &groupLocks, vector<DIWRSpinLock::Unlocker> &componentLocks)
{
    TaskProcessMessages(system, messageQueue);

    MessageBuilder messageBuilder;
    system.Update(env, messageBuilder);

    for (auto &[streamArchetype, stream] : messageBuilder.EntityAddedStreams()._data)
    {
        for (auto &entity : *stream)
        {
            AssignComponentIDs(ToArray(entity.components), _lastComponentId);

            ArchetypeFull archetype = ComputeArchetype(ToArray(entity.components));

            auto groupsLock = _archetypeGroupsLock.Lock(DIWRSpinLock::LockType::Read);

            optional<DIWRSpinLock::Unlocker> groupLock;
            DIWRSpinLock::Unlocker *groupLockRef = nullptr;
            bool isGroupWasLockedForRead = false;

            ArchetypeGroup *group;
            auto groupSearch = _archetypeGroupsFull.find(archetype);
            if (groupSearch == _archetypeGroupsFull.end())
            {
                group = &AddNewArchetypeGroup(archetype, ToArray(entity.components));
                // no need to lock the group, nobody can access it yet
            }
            else
            {
                group = &groupSearch->second;
                for (auto &lock : groupLocks)
                {
                    if (lock.first == archetype)
                    {
                        isGroupWasLockedForRead = true;
                        groupLockRef = &lock.second;
                        groupLockRef->Transition(DIWRSpinLock::LockType::Exclusive); // already locked, just transit it to Exclusive
                        break;
                    }
                }
                if (isGroupWasLockedForRead == false)
                {
                    // not locked, acquire a new lock
                    groupLock.emplace(group->lock.Lock(DIWRSpinLock::LockType::Exclusive));
                    groupLockRef = &*groupLock;
                }
            }

            groupsLock.Unlock();

            AddEntityToArchetypeGroup(archetype, *group, entity.entityID, ToArray(entity.components), nullptr);

            if (isGroupWasLockedForRead)
            {
                groupLockRef->Transition(DIWRSpinLock::LockType::Read); // just transit it back to Read
            }
            else if (groupLockRef)
            {
                groupLockRef->Unlock();
            }
        }
    }

    for (const auto &[streamArchetype, stream] : messageBuilder.EntityRemovedStreams()._data)
    {
        for (auto &entity : *stream)
        {
            auto locationsLock = _entitiesLocationsLock.Lock(DIWRSpinLock::LockType::Read);

            auto entityLocation = _entitiesLocations.find(entity);
            ASSUME(entityLocation != _entitiesLocations.end());

            auto &[group, index] = entityLocation->second;

            auto groupLock = group->lock.Lock(DIWRSpinLock::LockType::Exclusive);

            --group->entitiesCount;
            uiw replaceIndex = group->entitiesCount;

            bool isReplaceWithLast = replaceIndex != index;

            if (isReplaceWithLast)
            {
                // copy entity id
                group->entities[index] = group->entities[replaceIndex];

                // copy components data and optionally components ids
                for (uiw componentIndex = 0; componentIndex < group->uniqueTypedComponentsCount; ++componentIndex)
                {
                    auto &arr = group->components[componentIndex];
                    void *target = arr.data.get() + arr.sizeOf * index * arr.stride;
                    void *source = arr.data.get() + arr.sizeOf * replaceIndex * arr.stride;
                    uiw copySize = arr.sizeOf * arr.stride;
                    memcpy(target, source, copySize);

                    if (!arr.isUnique)
                    {
                        ui32 *idTarget = arr.ids.get() + index * arr.stride;
                        ui32 *idSource = arr.ids.get() + replaceIndex * arr.stride;
                        ui32 idCopySize = sizeof(ui32) * arr.stride;
                        memcpy(idTarget, idSource, idCopySize);
                    }
                }
            }

            locationsLock.Transition(DIWRSpinLock::LockType::Exclusive);

            groupLock.Unlock();

            // remove deleted entity location
            _entitiesLocations.erase(entityLocation);

            if (isReplaceWithLast)
            {
                // patch replaced entity's index
                entityLocation = _entitiesLocations.find(group->entities[index]);
                ASSUME(entityLocation != _entitiesLocations.end());
                entityLocation->second.index = index;
            }

            locationsLock.Unlock();
        }
    }

    for (auto &lock : componentLocks)
    {
        ASSUME(lock.LockType() == DIWRSpinLock::LockType::Inclusive);
        lock.Transition(DIWRSpinLock::LockType::Exclusive);
    }

    // send messages to other indirect systems

    for (auto &lock : componentLocks)
    {
        lock.Unlock();
    }
    componentLocks.clear();

    for (auto &lock : groupLocks)
    {
        lock.second.Unlock();
    }
    groupLocks.clear();

    --decrementAtCompletion;
}

void SystemsManager::ManagedIndirectSystem::MessageQueue::clear()
{
    entityAddedStreams.clear();
    entityRemovedStreams.clear();
}

bool ECSTest::SystemsManager::ManagedIndirectSystem::MessageQueue::empty() const
{
    return 
        entityAddedStreams.empty() && 
        entityRemovedStreams.empty();
}
