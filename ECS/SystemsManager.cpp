#include "PreHeader.hpp"
#include "SystemsManager.hpp"

namespace ECSTest
{
    class ECSEntities : public EntitiesStream
    {
        shared_ptr<const SystemsManager> _parent{};
        vector<ComponentDesc> _tempComponents{};
        decltype(SystemsManager::_entitiesLocations)::const_iterator _it{};
    
    public:
        ECSEntities(const shared_ptr<const SystemsManager> &parent) : _parent(parent)
        {
            ASSUME(_parent->_isPausedExecution);
            _it = _parent->_entitiesLocations.begin();
        }

        [[nodiscard]] virtual optional<StreamedEntity> Next() override
        {
            ASSUME(_parent->_isPausedExecution);

            if (_it == _parent->_entitiesLocations.end())
            {
                return {};
            }

            _tempComponents.clear();

            auto[group, entityIndex] = _it->second;

            for (uiw componentIndex = 0; componentIndex < group->uniqueTypedComponentsCount; ++componentIndex)
            {
                const auto &source = group->components[componentIndex];

                for (uiw offset = 0; offset < source.stride; ++offset)
                {
                    uiw index = _tempComponents.size();
                    _tempComponents.emplace_back();
                    auto &target = _tempComponents[index];

                    target.alignmentOf = source.alignmentOf;
                    target.isUnique = source.isUnique;
                    target.sizeOf = source.sizeOf;
                    target.type = source.type;
                    target.data = source.data.get() + entityIndex * source.sizeOf * source.stride + offset * source.sizeOf;
                }
            }

            StreamedEntity streamed;
            streamed.components = ToArray(_tempComponents);
            streamed.entityId = _it->first;

            ++_it;

            return streamed;
        }
    };
}

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

shared_ptr<SystemsManager> SystemsManager::New()
{
    struct Inherited : public SystemsManager
    {
        Inherited() : SystemsManager()
        {}
    };
    return make_shared<Inherited>();
}

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

	for (auto &pipeline : _pipelines)
	{
        if (isDirectSystem)
        {
            for (const auto &existingSystem : pipeline.directSystems)
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
            for (const auto &existingSystem : pipeline.indirectSystems)
            {
                if (existingSystem.system->Type() == system->Type())
                {
                    SOFTBREAK; // system with such type already exists
                    return;
                }
            }
        }
	}

    auto &pipeline = _pipelines[pipelineGroup.index];

    for (auto &otherPipeline : _pipelines)
    {
        if (&otherPipeline == &pipeline)
        {
            continue;
        }

        for (auto req : requestedComponents.writeAccess)
        {
            if (std::find(otherPipeline.writeComponents.begin(), otherPipeline.writeComponents.end(), req.type) != otherPipeline.writeComponents.end())
            {
                SOFTBREAK; // other pipeline already requested that component for write, that is not allowed
                return;
            }
        }
    }

    for (auto req : requestedComponents.writeAccess)
    {
        if (std::find(pipeline.writeComponents.begin(), pipeline.writeComponents.end(), req.type) == pipeline.writeComponents.end())
        {
            pipeline.writeComponents.push_back(req.type);
        }
    }

    _archetypeReflector.StartTrackingMatchingArchetypes((uiw)system.get(), requestedComponents.archetypeDefining);

    if (isDirectSystem)
    {
		ManagedDirectSystem direct;
        direct.executedAt = pipeline.executionFrame - 1;
		direct.system.reset(system.release()->AsDirectSystem());
        pipeline.directSystems.emplace_back(move(direct));
    }
    else
    {
		ManagedIndirectSystem indirect;
        indirect.executedAt = pipeline.executionFrame - 1;
		indirect.system.reset(system.release()->AsIndirectSystem());
        pipeline.indirectSystems.emplace_back(move(indirect));
    }
}

void SystemsManager::Unregister(StableTypeId systemType)
{
    if (_entitiesLocations.size())
    {
        HARDBREAK; // unregistering of the systems with non-empty scene is currently unsupported
        return;
    }

    auto recomputeWriteComponents = [](Pipeline &pipeline)
    {
        pipeline.writeComponents.clear();

        auto addForSystem = [&pipeline](System &system)
        {
            for (auto req : system.RequestedComponents().writeAccess)
            {
                if (std::find(pipeline.writeComponents.begin(), pipeline.writeComponents.end(), req.type) == pipeline.writeComponents.end())
                {
                    pipeline.writeComponents.push_back(req.type);
                }
            }
        };

        for (const auto &managed : pipeline.directSystems)
        {
            addForSystem(*managed.system);
        }
        for (const auto &managed : pipeline.indirectSystems)
        {
            addForSystem(*managed.system);
        }
    };

    for (auto &pipeline : _pipelines)
    {
        auto &directSystems = pipeline.directSystems;
        auto &indirectSystems = pipeline.indirectSystems;

        for (auto &managed : directSystems)
        {
            if (managed.system->Type() == systemType)
            {
                _archetypeReflector.StopTrackingMatchingArchetypes((uiw)managed.system.get());

                auto diff = &managed - &directSystems.front();
                directSystems.erase(directSystems.begin() + diff);
                recomputeWriteComponents(pipeline);
                return;
            }
        }
        for (auto &managed : pipeline.indirectSystems)
        {
            if (managed.system->Type() == systemType)
            {
                _archetypeReflector.StopTrackingMatchingArchetypes((uiw)managed.system.get());

                auto diff = &managed - &indirectSystems.front();
                indirectSystems.erase(indirectSystems.begin() + diff);
                recomputeWriteComponents(pipeline);
                return;
            }
        }
    }

    SOFTBREAK; // system not found
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
    for (const auto &component : components)
    {
        ASSUME(component.isUnique || component.id != 0);
    }
    return ArchetypeFull::Create<SerializedComponent, &SerializedComponent::type, &SerializedComponent::id>(components);
}

static void AssignComponentIDs(Array<SerializedComponent> components, std::atomic<ui32> &lastComponentId)
{
    for (auto &component : components)
    {
        if (!component.isUnique)
        {
            component.id = ++lastComponentId;
        }
        else
        {
            ASSUME(component.id == 0);
        }
    }
}

void SystemsManager::Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, vector<unique_ptr<EntitiesStream>> &&streams)
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
    _isPausedExecution = false;
    _isSchedulerPaused = false;

    _schedulerThread = std::thread([this, &streams] { StartScheduler(move(streams)); });
}

void SystemsManager::Pause(bool isWaitForStop)
{
    _isPausedExecution = true;

    if (isWaitForStop)
    {
        std::unique_lock waitLock{_schedulerPausedMutex};
        _schedulerPausedNotifier.wait(waitLock, [this] { return _isSchedulerPaused == true; });
    }
}

void SystemsManager::Resume()
{
    if (!_isPausedExecution)
    {
        SOFTBREAK;
        return;
    }

    if (_isStoppingExecution || _schedulerThread.joinable() == false)
    {
        SOFTBREAK;
        return;
    }

    _isPausedExecution = false;
    std::scoped_lock lock{_executionPauseMutex};
    _executionPauseNotifier.notify_all();
}

void SystemsManager::Stop(bool isWaitForStop)
{
    _isStoppingExecution = true;
    
    // making sure the scheduler thread is not paused
    _isPausedExecution = false;
    {
        std::scoped_lock lock{_executionPauseMutex};
        _executionPauseNotifier.notify_all();
    }

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

bool SystemsManager::IsRunning() const
{
    return _schedulerThread.joinable();
}

bool SystemsManager::IsPaused() const
{
    return _isPausedExecution;
}

//void SystemsManager::StreamIn(vector<unique_ptr<EntitiesStream>> &&streams)
//{
//    NOIMPL;
//}

shared_ptr<EntitiesStream> SystemsManager::StreamOut() const
{
    return make_shared<ECSEntities>(shared_from_this());
}

auto SystemsManager::FindArchetypeGroup(const ArchetypeFull &archetype, Array<const SerializedComponent> components) -> ArchetypeGroup &
{
    auto searchResult = _archetypeGroupsFull.find(archetype);
    if (searchResult != _archetypeGroupsFull.end())
    {
        return searchResult->second;
    }

    // such group doesn't exist yet, adding a new one
    return AddNewArchetypeGroup(archetype, components);
}

auto SystemsManager::AddNewArchetypeGroup(const ArchetypeFull &archetype, Array<const SerializedComponent> components) -> ArchetypeGroup &
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
    std::sort(uniqueTypes.begin(), uniqueTypes.end());

    group.archetype = archetype;

    group.uniqueTypedComponentsCount = (ui16)uniqueTypes.size();
    group.components = make_unique<ArchetypeGroup::ComponentArray[]>(group.uniqueTypedComponentsCount);

    for (const auto &component : components)
    {
        auto pred = [&component](const ArchetypeGroup::ComponentArray &stored)
        {
            return stored.sizeOf == 0 || stored.type == component.type;
        };
        
        auto searchResult = std::find_if(group.components.get(), group.components.get() + group.uniqueTypedComponentsCount, pred);
        ASSUME(searchResult != group.components.get() + group.uniqueTypedComponentsCount);
        auto &componentArray = *searchResult;

        componentArray.alignmentOf = component.alignmentOf;
        componentArray.sizeOf = component.sizeOf;
        componentArray.type = component.type;
        componentArray.isUnique = component.isUnique; // TODO: ensure uniqueness
        ++componentArray.stride;

        ASSUME(componentArray.stride == 1 || !componentArray.isUnique);
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
            uiw allocSize = sizeof(ui32) * componentArray.stride * group.reservedCount;
            componentArray.ids.reset((ui32 *)malloc(allocSize));
            memset(componentArray.ids.get(), 0xFF, allocSize);
        }
    }

    group.entities.reset((EntityID *)malloc(sizeof(EntityID) * group.reservedCount));

    // also add that archetype to the library
    _archetypeReflector.AddToLibrary(archetype.ToShort(), move(uniqueTypes));

    return group;
}

void SystemsManager::AddEntityToArchetypeGroup(const ArchetypeFull &archetype, ArchetypeGroup &group, EntityID entityId, Array<const SerializedComponent> components, MessageBuilder *messageBuilder)
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
                memset(newUPtr + componentArray.stride * group.entitiesCount, 0xFF, (group.reservedCount - group.entitiesCount) * sizeof(ui32) * componentArray.stride);
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
        auto findResult = std::find_if(group.components.get(), group.components.get() + group.uniqueTypedComponentsCount, pred);
        ASSUME(findResult != group.components.get() + group.uniqueTypedComponentsCount);
        auto &componentArray = *findResult;

        uiw offset = 0;
        if (!componentArray.isUnique)
        {
            for (; ; ++offset)
            {
                ASSUME(offset < componentArray.stride);
                if (componentArray.ids[componentArray.stride * group.entitiesCount + offset] == ui32_max)
                {
                    break;
                }
            }
            componentArray.ids[componentArray.stride * group.entitiesCount + offset] = component.id;
        }

        memcpy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * group.entitiesCount + offset * componentArray.sizeOf, component.data, componentArray.sizeOf);

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

void SystemsManager::StartScheduler(vector<unique_ptr<EntitiesStream>> &&streams)
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
        auto reflected = _archetypeReflector.Reflect(archetype);
        MessageStreamEntityAdded stream = {archetype, messages};
        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (ArchetypeReflector::Satisfies(reflected, managed.system->RequestedComponents().archetypeDefining))
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
        if (_isPausedExecution)
        {
            for (const auto &worker : _workerThreads)
            {
                std::unique_lock waitLock{_workerFinishedWorkNotifier->first};
                _workerFinishedWorkNotifier->second.wait(waitLock, [&worker] { return worker.WorkInProgressCount() == 0; });
            }

            _isSchedulerPaused = true;
            {
                std::scoped_lock lock{_schedulerPausedMutex};
                _schedulerPausedNotifier.notify_all();
            }

            std::unique_lock waitLock{_executionPauseMutex};
            _executionPauseNotifier.wait(waitLock, [this] { return _isPausedExecution == false; });

            _isSchedulerPaused = false;
        }
        else
        {
            SchedulerLoop();
        }
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
                                            groupLock->Unlock();
                                            return false;
                                        }
                                        managed.componentLocks.push_back({group.components[index].type, move(*componentLock)});
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
                                            groupLock->Unlock();
                                            return false;
                                        }
                                        managed.componentLocks.push_back({group.components[index].type, move(*componentLock)});
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
                                    groupLock->Unlock();
                                    return false;
                                }
                                managed.componentLocks.push_back({component.type, move(*componentLock)});
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
                    lock.second.Unlock();
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

        auto messageQueueLock = managed.messageQueueLock.Lock(DIWRSpinLock::LockType::Exclusive);
        auto movedOutMessageQueue = move(managed.messageQueue);
        managed.messageQueue = {};
        messageQueueLock.Unlock();

        auto work = std::bind(&SystemsManager::TaskExecuteIndirectSystem, this, std::ref(*managed.system), move(movedOutMessageQueue), env, std::ref(*pipeline.systemsAtExecution), std::ref(managed.groupLocks), std::ref(managed.componentLocks));
        FindBestWorker().AddWork(work);
        isAddedWork = true;
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
    for (const auto &stream : messageQueue.componentChangedStreams)
    {
        system.ProcessMessages(stream);
    }
    for (const auto &stream : messageQueue.entityRemovedStreams)
    {
        system.ProcessMessages(stream);
    }
}

void SystemsManager::TaskExecuteIndirectSystem(IndirectSystem &system, ManagedIndirectSystem::MessageQueue messageQueue, System::Environment env, std::atomic<ui32> &decrementAtCompletion, vector<pair<ArchetypeFull, DIWRSpinLock::Unlocker>> &groupLocks, vector<pair<StableTypeId, DIWRSpinLock::Unlocker>> &componentLocks)
{
    TaskProcessMessages(system, messageQueue);

    MessageBuilder messageBuilder;
    system.Update(env, messageBuilder);

    auto lockOrTransition = [&groupLocks](const ArchetypeFull &archetype) -> auto
    {
        class ConditionalLock
        {
            bool _isTransitioned{};
            optional<DIWRSpinLock::Unlocker> _lock{};
            DIWRSpinLock::Unlocker *_lockRef{};

        public:
            ~ConditionalLock()
            {
                ASSUME(!_lockRef && !_lock);
            }

            ConditionalLock(vector<pair<ArchetypeFull, DIWRSpinLock::Unlocker>> &groupLocks, const ArchetypeFull &archetype)
            {
                for (auto &lock : groupLocks)
                {
                    if (lock.first == archetype)
                    {
                        _isTransitioned = true;
                        _lockRef = &lock.second;
                        break;
                    }
                }
            }

            void Lock(DIWRSpinLock &spinLock)
            {
                if (_isTransitioned)
                {
                    _lockRef->Transition(DIWRSpinLock::LockType::Exclusive); // already locked, just transit it to Exclusive
                }
                else
                {
                    _lock.emplace(spinLock.Lock(DIWRSpinLock::LockType::Exclusive));
                }
            }

            void Unlock()
            {
                if (_isTransitioned)
                {
                    _lockRef->Transition(DIWRSpinLock::LockType::Read); // just transit it back to Read
                    _lockRef = nullptr;
                }
                else
                {
                    _lock->Unlock();
                    _lock.reset();
                }
            }
        };
        return ConditionalLock(groupLocks, archetype);
    };

    for (auto &[streamArchetype, stream] : messageBuilder.EntityAddedStreams()._data)
    {
        for (auto &entity : *stream)
        {
            AssignComponentIDs(ToArray(entity.components), _lastComponentId);

            ArchetypeFull archetype = ComputeArchetype(ToArray(entity.components));

            auto groupsLock = _archetypeGroupsLock.Lock(DIWRSpinLock::LockType::Read);

            // locking the group for Exclusive will ensure that every other group reading or
            // writing to that group has already finished running
            auto groupConditionalLock = lockOrTransition(archetype);

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
                groupConditionalLock.Lock(group->lock);
            }

            groupsLock.Unlock();

            AddEntityToArchetypeGroup(archetype, *group, entity.entityID, ToArray(entity.components), nullptr);

            groupConditionalLock.Unlock();
        }
    }

    for (auto &lock : componentLocks)
    {
        ASSUME(lock.second.LockType() == DIWRSpinLock::LockType::Inclusive);
        lock.second.Transition(DIWRSpinLock::LockType::Exclusive);
    }

    for (const auto &[componentType, stream] : messageBuilder.ComponentChangedStreams()._data)
    {
        auto locationsLock = _entitiesLocationsLock.Lock(DIWRSpinLock::LockType::Read);

        for (const auto &info : stream->infos)
        {
            auto entityLocation = _entitiesLocations.find(info.entityID);
            ASSUME(entityLocation != _entitiesLocations.end());

            auto &[group, entityIndex] = entityLocation->second;

            auto groupConditionalLock = lockOrTransition(group->archetype);
            groupConditionalLock.Lock(group->lock);

            uiw componentIndex = 0;
            for (; ; ++componentIndex)
            {
                ASSUME(componentIndex < group->uniqueTypedComponentsCount);
                if (group->components[componentIndex].type == componentType)
                {
                    break;
                }
            }

            auto &componentArray = group->components[componentIndex];

            ASSUME(info.component.alignmentOf == componentArray.alignmentOf);
            ASSUME(info.component.isUnique == componentArray.isUnique);
            ASSUME(info.component.sizeOf == componentArray.sizeOf);
            ASSUME(info.component.type == componentArray.type);

            uiw offset = 0;
            if (!info.component.isUnique)
            {
                for (; ; ++offset)
                {
                    ASSUME(offset < componentArray.stride);
                    if (info.component.id == componentArray.ids[entityIndex * componentArray.stride + offset])
                    {
                        break;
                    }
                }
            }

            // don't lock here because every component that you change is already supposed to be locked
            auto predicate = [type = componentArray.type](const auto &contained)
            {
                return contained.first == type;
            };
            ASSUME(std::find_if(componentLocks.begin(), componentLocks.end(), predicate) != componentLocks.end());

            memcpy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * entityIndex + componentArray.sizeOf * offset, info.component.data, info.component.sizeOf);

            groupConditionalLock.Unlock();
        }

        locationsLock.Unlock();
    }

    for (const auto &[streamArchetype, stream] : messageBuilder.EntityRemovedStreams()._data)
    {
        auto locationsLock = _entitiesLocationsLock.Lock(DIWRSpinLock::LockType::Read);

        for (auto &entity : *stream)
        {
            auto entityLocation = _entitiesLocations.find(entity);
            ASSUME(entityLocation != _entitiesLocations.end());

            auto &[group, index] = entityLocation->second;

            // locking the group for Exclusive will ensure that every other group reading or
            // writing to that group has already finished running
            auto groupConditionalLock = lockOrTransition(group->archetype);
            groupConditionalLock.Lock(group->lock);

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

            groupConditionalLock.Unlock();

            // remove deleted entity location
            _entitiesLocations.erase(entityLocation);

            if (isReplaceWithLast)
            {
                // patch replaced entity's index
                entityLocation = _entitiesLocations.find(group->entities[index]);
                ASSUME(entityLocation != _entitiesLocations.end());
                entityLocation->second.index = index;
            }

            locationsLock.Transition(DIWRSpinLock::LockType::Read);
        }

        locationsLock.Unlock();
    }

    for (auto &[streamArchetype, streamPointer] : messageBuilder.EntityAddedStreams()._data)
    {
        auto reflected = _archetypeReflector.Reflect(streamArchetype);
        auto stream = MessageStreamEntityAdded(streamArchetype, streamPointer);

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != &system && ArchetypeReflector::Satisfies(reflected, managed.system->RequestedComponents().archetypeDefining))
                {
                    auto messageQueueLock = managed.messageQueueLock.Lock(DIWRSpinLock::LockType::Exclusive);
                    managed.messageQueue.entityAddedStreams.emplace_back(stream);
                    messageQueueLock.Unlock();
                }
            }
        }
    }

    for (const auto &[componentType, streamPointer] : messageBuilder.ComponentChangedStreams()._data)
    {
        auto stream = MessageStreamComponentChanged(componentType, streamPointer);

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != &system)
                {
                    auto requested = managed.system->RequestedComponents();
                    auto searchPredicate = [componentType](const System::RequestedComponent &stored) { return componentType == stored.type; };

                    if (requested.subtractive.find(searchPredicate) != requested.subtractive.end())
                    {
                        continue;
                    }

                    if (requested.required.empty() ||
                        requested.required.find(searchPredicate) != requested.required.end() ||
                        requested.optional.find(searchPredicate) != requested.optional.end())
                    {
                        auto messageQueueLock = managed.messageQueueLock.Lock(DIWRSpinLock::LockType::Exclusive);
                        managed.messageQueue.componentChangedStreams.emplace_back(stream);
                        messageQueueLock.Unlock();
                    }
                }
            }
        }
    }

    for (const auto &[streamArchetype, streamPointer] : messageBuilder.EntityRemovedStreams()._data)
    {
        auto reflected = _archetypeReflector.Reflect(streamArchetype);
        auto stream = MessageStreamEntityRemoved(streamArchetype, streamPointer);

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != &system && ArchetypeReflector::Satisfies(reflected, managed.system->RequestedComponents().archetypeDefining))
                {
                    auto messageQueueLock = managed.messageQueueLock.Lock(DIWRSpinLock::LockType::Exclusive);
                    managed.messageQueue.entityRemovedStreams.emplace_back(stream);
                    messageQueueLock.Unlock();
                }
            }
        }
    }

    for (auto &lock : componentLocks)
    {
        lock.second.Unlock();
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
    componentChangedStreams.clear();
    entityRemovedStreams.clear();
}

bool ECSTest::SystemsManager::ManagedIndirectSystem::MessageQueue::empty() const
{
    return 
        entityAddedStreams.empty() && 
        componentChangedStreams.empty() &&
        entityRemovedStreams.empty();
}
