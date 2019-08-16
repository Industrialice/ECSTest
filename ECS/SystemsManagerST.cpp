#include "PreHeader.hpp"
#include "SystemsManagerST.hpp"

namespace ECSTest
{
    class ECSEntitiesST : public IEntitiesStream
    {
        std::weak_ptr<const SystemsManagerST> _parent{};
        vector<ComponentDesc> _tempComponents{};
        decltype(SystemsManagerST::_entitiesLocations)::const_iterator _it{};

    public:
        ECSEntitiesST(const shared_ptr<const SystemsManagerST> &parent) : _parent(parent)
        {
            ASSUME(parent->_isPausedExecution);
            _it = parent->_entitiesLocations.begin();
        }

        [[nodiscard]] virtual optional<StreamedEntity> Next() override
        {
            auto locked = _parent.lock();
            ASSUME(locked);

            ASSUME(locked->_isPausedExecution);

            if (_it == locked->_entitiesLocations.end())
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
                    target.isTag = false;
                    target.sizeOf = source.sizeOf;
                    target.type = source.type;
                    target.data = source.data.get() + entityIndex * source.sizeOf * source.stride + offset * source.sizeOf;
                }
            }
			
            for (uiw tagIndex = 0; tagIndex < group->tagsCount; ++tagIndex)
            {
                uiw index = _tempComponents.size();
                _tempComponents.emplace_back();
                auto &target = _tempComponents[index];

                target.isUnique = true;
                target.isTag = true;
                target.type = group->tags[tagIndex];
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

SystemsManagerST::SystemsManagerST(const shared_ptr<LoggerType> &logger) : _logger(logger)
{
    if (_logger == nullptr)
    {
        _logger = make_shared<LoggerType>();
    }
}

shared_ptr<SystemsManagerST> SystemsManagerST::New(const shared_ptr<LoggerType> &logger)
{
    struct Inherited : public SystemsManagerST
    {
        Inherited(const shared_ptr<LoggerType> &logger) : SystemsManagerST(logger)
        {}
    };
    return make_shared<Inherited>(logger);
}

auto SystemsManagerST::CreatePipeline(optional<TimeDifference> executionStep, bool isMergeIfSuchPipelineExists) -> Pipeline
{
    if (executionStep && executionStep->ToMSec() < 0)
    {
        HARDBREAK; // negative execution step
        executionStep = {};
    }

    Pipeline group;
    if (isMergeIfSuchPipelineExists)
    {
        for (ui32 index = 0; index < static_cast<ui32>(_pipelines.size()); ++index)
        {
            if (_pipelines[index].executionStep == executionStep)
            {
                group.index = index;
                return group;
            }
        }
    }
    group.index = static_cast<ui32>(_pipelines.size());
    _pipelines.emplace_back();
    _pipelines.back().executionStep = executionStep;
    return group;
}

auto SystemsManagerST::GetPipelineInfo(Pipeline pipeline) const -> PipelineInfo
{
    ASSUME(pipeline.index < _pipelines.size());

    const auto &pipelineData = _pipelines[pipeline.index];

    PipelineInfo info;
    info.executedTimes = pipelineData.executionFrame;
    info.directSystems = static_cast<ui32>(pipelineData.directSystems.size());
    info.indirectSystems = static_cast<ui32>(pipelineData.indirectSystems.size());
    info.executionStep = pipelineData.executionStep;
	info.timeSpentExecuting = pipelineData.timeSpentExecuting;

    return info;
}

auto SystemsManagerST::GetManagerInfo() const -> ManagerInfo
{
    ManagerInfo info;
    info.isMultiThreaded = false;
    info.timeSinceStart = _timeSinceStartAtomic;
    return info;
}

void SystemsManagerST::SetLogger(const shared_ptr<LoggerType> &logger)
{
    if (Funcs::AreSharedPointersEqual(_logger, logger))
    {
        return;
    }

    _logger = logger;
    if (!_logger)
    {
        _logger = make_shared<LoggerType>();
    }
}

void SystemsManagerST::Register(unique_ptr<System> system, Pipeline pipeline)
{
	ASSUME(system);

	if (_entitiesLocations.size())
	{
		HARDBREAK; // registering of the systems with non-empty scene is currently unsupported
		return;
	}

	ASSUME(pipeline.index < _pipelines.size());

	bool isDirectSystem = system->AsDirectSystem() != nullptr;
	auto requestedComponents = system->RequestedComponents();

	if (isDirectSystem)
	{
		if (requestedComponents.withData.empty())
		{
            _logger->Message(LogLevels::Error, selfName, "The system will never be executed with such configuration");
			return;
		}
	}

	for (auto &pipelineData : _pipelines)
	{
		if (isDirectSystem)
		{
			for (const auto &existingSystem : pipelineData.directSystems)
			{
				if (existingSystem.system->GetTypeId() == system->GetTypeId())
				{
                    _logger->Message(LogLevels::Error, selfName, "System with such type already exists");
					return;
				}
			}
		}
		else
		{
			for (const auto &existingSystem : pipelineData.indirectSystems)
			{
				if (existingSystem.system->GetTypeId() == system->GetTypeId())
				{
                    _logger->Message(LogLevels::Error, selfName, "System with such type already exists");
					return;
				}
			}
		}
	}

	auto &pipelineData = _pipelines[pipeline.index];

	for (auto &otherPipeline : _pipelines)
	{
		if (&otherPipeline == &pipelineData)
		{
			continue;
		}

		for (auto req : requestedComponents.writeAccess)
		{
			if (std::find(otherPipeline.writeComponents.begin(), otherPipeline.writeComponents.end(), req.type) != otherPipeline.writeComponents.end())
			{
                _logger->Message(LogLevels::Error, selfName, "Other pipeline already requested that component for write, that is not allowed");
				return;
			}
		}
	}

	for (auto req : requestedComponents.writeAccess)
	{
		if (std::find(pipelineData.writeComponents.begin(), pipelineData.writeComponents.end(), req.type) == pipelineData.writeComponents.end())
		{
			pipelineData.writeComponents.push_back(req.type);
		}
	}

	_archetypeReflector.StartTrackingMatchingArchetypes(reinterpret_cast<uiw>(system.get()), requestedComponents.archetypeDefiningInfoOnly);

    auto addSystem = [&pipelineData](auto &managed, auto *system)
    {
        managed.executedAt = pipelineData.executionFrame - 1;
        managed.system.reset(system);
    };
    
    if (isDirectSystem)
	{
		ManagedDirectSystem direct;
        addSystem(direct, system.release()->AsDirectSystem());
		pipelineData.directSystems.emplace_back(move(direct));
	}
	else
	{
		ManagedIndirectSystem indirect;
        addSystem(indirect, system.release()->AsIndirectSystem());
		pipelineData.indirectSystems.emplace_back(move(indirect));
	}
}

void SystemsManagerST::Unregister(TypeId systemType)
{
	if (_entitiesLocations.size())
	{
		HARDBREAK; // unregistering of the systems with non-empty scene is currently unsupported
		return;
	}

	auto recomputeWriteComponents = [](PipelineData &pipeline)
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
			if (managed.system->GetTypeId() == systemType)
			{
				_archetypeReflector.StopTrackingMatchingArchetypes(reinterpret_cast<uiw>(managed.system.get()));

				auto diff = &managed - &directSystems.front();
				directSystems.erase(directSystems.begin() + diff);
				recomputeWriteComponents(pipeline);
				return;
			}
		}
		for (auto &managed : pipeline.indirectSystems)
		{
			if (managed.system->GetTypeId() == systemType)
			{
				_archetypeReflector.StopTrackingMatchingArchetypes(reinterpret_cast<uiw>(managed.system.get()));

				auto diff = &managed - &indirectSystems.front();
				indirectSystems.erase(indirectSystems.begin() + diff);
				recomputeWriteComponents(pipeline);
				return;
			}
		}
	}

    _logger->Message(LogLevels::Error, selfName, "System not found");
}

static void StreamedToSerialized(Array<const IEntitiesStream::ComponentDesc> streamed, vector<SerializedComponent> &serialized)
{
	serialized.resize(streamed.size());
	for (uiw index = 0; index < streamed.size(); ++index)
	{
		auto &s = streamed[index];
		auto &t = serialized[index];

		t.alignmentOf = s.alignmentOf;
		t.data = s.data;
        t.id = {};
		t.isUnique = s.isUnique;
        t.isTag = s.isTag;
		t.sizeOf = s.sizeOf;
		t.type = s.type;
	}
}

static ArchetypeFull ComputeArchetype(Array<const SerializedComponent> components)
{
	for (const auto &component : components)
	{
		ASSUME(component.isUnique || component.id);
	}
	return ArchetypeFull::Create<SerializedComponent, ComponentDescription, &SerializedComponent::type, &SerializedComponent::id>(components);
}

static void AssignComponentIDs(Array<SerializedComponent> components, ComponentIDGenerator &idGenerator)
{
	for (auto &component : components)
	{
		if (!component.isUnique)
		{
            if (component.id.IsValid() == false)
            {
                component.id = idGenerator.Generate();
            }
		}
		else
		{
			ASSUME(component.id.IsValid() == false);
		}
	}
}

void SystemsManagerST::Start(AssetsManager &&assetsManager, EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, vector<unique_ptr<IEntitiesStream>> &&streams)
{
	ASSUME(_entitiesLocations.empty() && _schedulerThread.get_id() == std::thread::id{});

	if (workers.size())
	{
        _logger->Message(LogLevels::Warning, selfName, "Workers aren't used by a single threaded systems manager");
		workers.clear();
	}

	_assetsManager = move(assetsManager);
	_entityIdGenerator = move(idGenerator);

	_isStoppingExecution = false;
	_isPausedExecution = false;
	_isSchedulerPaused = false;

	_schedulerThread = std::thread([this, streams = move(streams)]() mutable { StartScheduler(streams); });
}

void SystemsManagerST::Pause(bool isWaitForStop)
{
	_isPausedExecution = true;

	if (isWaitForStop)
	{
		std::unique_lock waitLock{_schedulerPausedMutex};
		_schedulerPausedNotifier.wait(waitLock, [this] { return _isSchedulerPaused == true; });
	}
}

void SystemsManagerST::Resume()
{
	if (!_isPausedExecution)
	{
        _logger->Message(LogLevels::Error, selfName, "Cannot resume because the execution is already running");
		return;
	}

	if (_isStoppingExecution || _schedulerThread.joinable() == false)
	{
        _logger->Message(LogLevels::Error, selfName, "Cannot resume because the system is not started");
		return;
	}

	_isPausedExecution = false;
	std::scoped_lock lock{_executionPauseMutex};
	_executionPauseNotifier.notify_all();
}

void SystemsManagerST::Stop(bool isWaitForStop)
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
		ASSUME(_schedulerThread.get_id() != std::this_thread::get_id());
		_schedulerThread.join();
	}

	_entitiesLocations = {};
	_archetypeGroups = {};
	//_archetypeGroupsComponents = {};
	std::exchange(_archetypeGroupsFull, {});
	_archetypeReflector = {};
    _entityIdGenerator = {};
    _componentIdGenerator = {};
}

bool SystemsManagerST::IsRunning() const
{
	return _schedulerThread.joinable();
}

bool SystemsManagerST::IsPaused() const
{
	return _isPausedExecution;
}

shared_ptr<IEntitiesStream> SystemsManagerST::StreamOut() const
{
    return make_shared<ECSEntitiesST>(shared_from_this());
}

auto SystemsManagerST::FindArchetypeGroup(const ArchetypeFull &archetype, Array<const SerializedComponent> components) -> ArchetypeGroup &
{
	auto searchResult = _archetypeGroupsFull.find(archetype);
	if (searchResult != _archetypeGroupsFull.end())
	{
		return searchResult->second;
	}

	// such group doesn't exist yet, adding a new one
	return AddNewArchetypeGroup(archetype, components);
}

auto SystemsManagerST::AddNewArchetypeGroup(const ArchetypeFull &archetype, Array<const SerializedComponent> components) -> ArchetypeGroup &
{
	auto[insertedWhere, insertedResult] = _archetypeGroupsFull.try_emplace(archetype);
	ASSUME(insertedResult);
	ArchetypeGroup &group = insertedWhere->second;

	_archetypeGroups[archetype.ToShort()].emplace_back(std::ref(group));

	vector<TypeId> uniqueTypes;
	for (const auto &component : components)
	{
        if (component.isTag == false)
        {
            if (std::find(uniqueTypes.begin(), uniqueTypes.end(), component.type) == uniqueTypes.end())
            {
                uniqueTypes.push_back(component.type);
            }
        }
	}
	std::sort(uniqueTypes.begin(), uniqueTypes.end());

	group.archetype = archetype;

	group.uniqueTypedComponentsCount = static_cast<ui16>(uniqueTypes.size());
	group.components = make_unique<ArchetypeGroup::ComponentArray[]>(group.uniqueTypedComponentsCount);

	for (const auto &component : components)
	{
        if (component.isTag)
        {
            continue;
        }

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

	group.entitiesReservedCount = 8; // by default initialize enough space for 8 entities

	for (ui16 index = 0; index < group.uniqueTypedComponentsCount; ++index)
	{
		auto &componentArray = group.components[index];

		// allocate memory
		ASSUME(componentArray.sizeOf > 0 && componentArray.stride > 0 && group.entitiesReservedCount > 0 && componentArray.alignmentOf > 0);
		componentArray.data.reset(Allocator::MallocAlignedRuntime::Allocate(componentArray.sizeOf * componentArray.stride * group.entitiesReservedCount, componentArray.alignmentOf));

		if (!componentArray.isUnique)
		{
			componentArray.ids.reset(Allocator::Malloc::Allocate<ComponentID>(componentArray.stride * group.entitiesReservedCount));
		}
	}

	group.entities.reset(Allocator::Malloc::Allocate<EntityID>(group.entitiesReservedCount));
    
    // add tag components
    vector<TypeId> tagTypes;
    for (const auto &component : components)
    {
        if (component.isTag)
        {
            tagTypes.push_back(component.type);
        }
    }
    group.tagsCount = static_cast<ui16>(tagTypes.size());
    group.tags = make_unique<TypeId[]>(group.tagsCount);
    std::copy(tagTypes.begin(), tagTypes.end(), group.tags.get());

	// also add that archetype to the library
    uniqueTypes.insert(uniqueTypes.end(), group.tags.get(), group.tags.get() + group.tagsCount);
    std::sort(uniqueTypes.begin(), uniqueTypes.end());
	_archetypeReflector.AddToLibrary(archetype.ToShort(), move(uniqueTypes));

	return group;
}

void SystemsManagerST::AddEntityToArchetypeGroup(const ArchetypeFull &archetype, ArchetypeGroup &group, EntityID entityId, Array<const SerializedComponent> components, MessageBuilder *messageBuilder)
{
    ASSUME(group.entitiesReservedCount);

	if (group.entitiesCount == group.entitiesReservedCount)
	{
		group.entitiesReservedCount *= 2;

		for (ui16 index = 0; index < group.uniqueTypedComponentsCount; ++index)
		{
			auto &componentArray = group.components[index];

			ASSUME(componentArray.sizeOf > 0 && componentArray.stride > 0 && componentArray.alignmentOf > 0);

			byte *oldPtr = componentArray.data.release();
			byte *newPtr = Allocator::MallocAlignedRuntime::Reallocate(oldPtr, componentArray.sizeOf * componentArray.stride * group.entitiesReservedCount, componentArray.alignmentOf);
			componentArray.data.reset(newPtr);

			if (!componentArray.isUnique)
			{
				ComponentID *oldUPtr = componentArray.ids.release();
				ComponentID *newUPtr = Allocator::Malloc::Reallocate(oldUPtr, componentArray.stride * group.entitiesReservedCount);
				componentArray.ids.reset(newUPtr);
			}
		}

		EntityID *oldPtr = group.entities.release();
		EntityID *newPtr = Allocator::Malloc::Reallocate(oldPtr, group.entitiesReservedCount + 1);
		group.entities.reset(newPtr);

		newPtr[group.entitiesReservedCount] = EntityID(); // use an extra entry to speed up predictive lookups
	}

	optional<std::reference_wrapper<ComponentArrayBuilder>> componentBuilder;
	if (messageBuilder)
	{
		componentBuilder = messageBuilder->AddEntity(entityId);
	}

    uiw tagsCount = 0;

	for (uiw index = 0; index < group.uniqueTypedComponentsCount; ++index)
	{
		if (group.components[index].isUnique == false)
		{
			MemOps::Set(group.components[index].ids.get() + group.entitiesCount * group.components[index].stride, ComponentID::InvalidID() & 0xFF, sizeof(ComponentID) * group.components[index].stride);
		}
	}

	for (uiw index = 0; index < components.size(); ++index)
	{
		const auto &component = components[index];

        if (component.isTag)
        {
            ++tagsCount;
            ASSUME(component.data == nullptr);
            ASSUME(std::find(group.tags.get(), group.tags.get() + group.tagsCount, component.type) != group.tags.get() + group.tagsCount);
        }
        else
        {
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
                ASSUME(component.isUnique == false);
                ASSUME(component.id);

                for (; ; ++offset)
                {
                    ASSUME(offset < componentArray.stride);
                    if (componentArray.ids[componentArray.stride * group.entitiesCount + offset].IsValid() == false)
                    {
                        break;
                    }
                }
                componentArray.ids[componentArray.stride * group.entitiesCount + offset] = component.id;
            }
            else
            {
                ASSUME(component.isUnique == true);
                ASSUME(component.id.IsValid() == false);
            }

            MemOps::Copy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * group.entitiesCount + offset * componentArray.sizeOf, component.data, componentArray.sizeOf);
        }

		if (componentBuilder)
		{
			componentBuilder->get().AddComponent(component);
		}
	}

    ASSUME(tagsCount == group.tagsCount);

	group.entities[group.entitiesCount] = entityId;

	_entitiesLocations[entityId] = {&group, group.entitiesCount};

	++group.entitiesCount;
}

void SystemsManagerST::StartScheduler(vector<unique_ptr<IEntitiesStream>> &streams)
{
	ASSUME(_tempMessageBuilder.IsEmpty());
    _tempMessageBuilder.SourceName("Initial Streaming");
	vector<SerializedComponent> serialized;

    auto before = TimeMoment::Now();
	for (auto &stream : streams)
	{
		while (const auto &entity = stream->Next())
		{
			StreamedToSerialized(entity->components, serialized);
			AssignComponentIDs(ToArray(serialized), _componentIdGenerator);
			ArchetypeFull entityArchetype = ComputeArchetype(ToArray(serialized));
			ArchetypeGroup &archetypeGroup = FindArchetypeGroup(entityArchetype, ToArray(serialized));
			AddEntityToArchetypeGroup(entityArchetype, archetypeGroup, entity->entityId, ToArray(serialized), &_tempMessageBuilder);
		}
		stream = {};
	}
    auto after = TimeMoment::Now();
    if (streams.size())
    {
        _logger->Message(LogLevels::Info, selfName, "Creating ECS structure took %.2lfs\n", (after - before).ToSec_f64());
		streams.clear();
    }

	auto &entityAddedStreams = _tempMessageBuilder.EntityAddedStreams();
	for (auto &[archetype, messages] : entityAddedStreams._data)
	{
		auto reflected = _archetypeReflector.Reflect(archetype);
		MessageStreamEntityAdded stream = {archetype, messages, _tempMessageBuilder.SourceName()};
		for (auto &pipeline : _pipelines)
		{
			for (auto &managed : pipeline.indirectSystems)
			{
				if (ArchetypeReflector::Satisfies(reflected, managed.system->RequestedComponents().archetypeDefiningInfoOnly))
				{
					managed.messageQueue.entityAddedStreams.push_back(stream);
				}
			}
		}
	}
	
	_tempMessageBuilder.Clear();

    _currentTime = TimeMoment::Now();

	while (!_isStoppingExecution)
	{
		if (_isPausedExecution)
		{
			_isSchedulerPaused = true;
			{
				std::scoped_lock lock{_schedulerPausedMutex};
				_schedulerPausedNotifier.notify_all();
			}

			std::unique_lock waitLock{_executionPauseMutex};
			_executionPauseNotifier.wait(waitLock, [this] { return _isPausedExecution == false; });

			_isSchedulerPaused = false;
            _currentTime = TimeMoment::Now();

            for (auto &pipeline : _pipelines)
            {
                pipeline.lastExecutedTime = {};
            }
		}
		else
		{
			SchedulerLoop();
		}
	}

	#ifdef PLATFORM_ANDROID
		DetachCurrentThread();
	#endif
}

void SystemsManagerST::SchedulerLoop()
{
    auto updateTimes = [this](MovableAtomic<TimeDifference> *timeSpentExecuting) -> TimeMoment
    {
        auto currentTime = TimeMoment::Now();
		auto delta = currentTime - _currentTime;
        _timeSinceStart += delta;
		if (timeSpentExecuting)
		{
			timeSpentExecuting->store(timeSpentExecuting->load() + delta);
		}
        _currentTime = currentTime;
        return currentTime;
    };

    uiw repeatedIndex = uiw_max;
    ui32 repeadedCount = 0;
    bool isTimeUpToDate = false;

    for (uiw index = 0, size = _pipelines.size(); index < size; )
    {
        auto &pipeline = _pipelines[index];

        if (pipeline.executionStep && pipeline.lastExecutedTime.HasValue())
        {
            auto nextExecution = pipeline.lastExecutedTime + *pipeline.executionStep;
            if (_currentTime < nextExecution)
            {
                ++index;
                continue;
            }
        }

		TimeDifference timeSinceLastFrame;
		if (pipeline.lastExecutedTime.HasValue())
		{
			timeSinceLastFrame = TimeMoment::Now() - pipeline.lastExecutedTime;
		}

        ExecutePipeline(pipeline, timeSinceLastFrame);

        auto currentTime = updateTimes(&pipeline.timeSpentExecuting);
        isTimeUpToDate = true;

        if (pipeline.executionStep)
        {
            if (pipeline.lastExecutedTime.HasValue())
            {
                pipeline.lastExecutedTime += *pipeline.executionStep;
            }
            else
            {
                pipeline.lastExecutedTime = currentTime;
            }

            if (repeatedIndex != index)
            {
                repeatedIndex = index;
                repeadedCount = 1;
            }
            else
            {
                ++repeadedCount;
                if (repeadedCount > 100)
                {
                    SOFTBREAK; // stuck in the loop, should slow execution down
                    ++index;
                }
            }
        }
        else
        {
            pipeline.lastExecutedTime = currentTime;
            ++index;
        }
    }

    if (!isTimeUpToDate)
    {
        updateTimes(nullptr);
    }
    _timeSinceStartAtomic = _timeSinceStart;
}

void SystemsManagerST::ExecutePipeline(PipelineData &pipeline, TimeDifference timeSinceLastFrame)
{
    for (auto &managed : pipeline.directSystems)
    {
        ASSUME(_tempMessageBuilder.IsEmpty());

        System::Environment env =
        {
            timeSinceLastFrame.ToSec(),
            pipeline.executionFrame,
            _timeSinceStart,
            managed.system->GetTypeId(),
            _entityIdGenerator,
            _componentIdGenerator,
            _tempMessageBuilder,
            LoggerWrapper(_logger.get(), managed.system->GetTypeName()),
            managed.system->GetKeyController(),
			_assetsManager
        };
        env.messageBuilder.SourceName(managed.system->GetTypeId().Name());

        IKeyController::ListenerHandle inputHandle;
        if (env.keyController)
        {
            inputHandle = env.keyController->OnControlAction(std::bind(&System::ControlInput, managed.system.get(), std::ref(env), _1));
        }

        managed.executedAt = pipeline.executionFrame;

        if (managed.executedTimes == 0)
        {
            IKeyController::ListenerHandle addToQueueHandle;
            if (env.keyController)
            {
                addToQueueHandle = env.keyController->OnControlAction(std::bind(&SendControlActionToQueue, std::ref(managed.controlsToSendQueue), _1));
            }

            managed.system->OnCreate(env);
            PassControlsToOtherSystemsAndClear(managed.controlsToSendQueue, managed.system.get());
            PatchComponentAddedMessages(env.messageBuilder);
            PatchEntityRemovedArchetypes(env.messageBuilder);
            UpdateECSFromMessages(env.messageBuilder);
            PassMessagesToIndirectSystemsAndClear(env.messageBuilder, nullptr);

            managed.system->OnInitialized(env);
            PassControlsToOtherSystemsAndClear(managed.controlsToSendQueue, managed.system.get());
            PatchComponentAddedMessages(env.messageBuilder);
            PatchEntityRemovedArchetypes(env.messageBuilder);
            UpdateECSFromMessages(env.messageBuilder);
            PassMessagesToIndirectSystemsAndClear(env.messageBuilder, nullptr);
        }

        ExecuteDirectSystem(*managed.system, managed.controlsReceivedQueue, managed.controlsToSendQueue, env);

        ++managed.executedTimes;
    }

    for (auto &managed : pipeline.indirectSystems)
    {
        ASSUME(_tempMessageBuilder.IsEmpty());

        System::Environment env =
        {
            timeSinceLastFrame.ToSec(),
            pipeline.executionFrame,
            _timeSinceStart,
            managed.system->GetTypeId(),
            _entityIdGenerator,
            _componentIdGenerator,
            _tempMessageBuilder,
            LoggerWrapper(_logger.get(), managed.system->GetTypeName()),
            managed.system->GetKeyController(),
			_assetsManager
        };
        env.messageBuilder.SourceName(managed.system->GetTypeId().Name());

        IKeyController::ListenerHandle inputHandle;
        if (env.keyController)
        {
            inputHandle = env.keyController->OnControlAction(std::bind(&System::ControlInput, managed.system.get(), std::ref(env), _1));
        }

        managed.executedAt = pipeline.executionFrame;

        if (managed.executedTimes == 0)
        {
            IKeyController::ListenerHandle addToQueueHandle;
            if (env.keyController)
            {
                addToQueueHandle = env.keyController->OnControlAction(std::bind(&SendControlActionToQueue, std::ref(managed.controlsToSendQueue), _1));
            }

            managed.system->OnCreate(env);
            PassControlsToOtherSystemsAndClear(managed.controlsToSendQueue, managed.system.get());
            PatchComponentAddedMessages(env.messageBuilder);
            PatchEntityRemovedArchetypes(env.messageBuilder);
            UpdateECSFromMessages(env.messageBuilder);
            PassMessagesToIndirectSystemsAndClear(env.messageBuilder, managed.system.get());

            auto before = TimeMoment::Now();
            ProcessMessagesAndClear(*managed.system, managed.messageQueue, env);
            managed.system->OnInitialized(env);
            auto after = TimeMoment::Now();
            _logger->Message(LogLevels::Info, selfName, "Initializing %*s took %.2lfs\n", static_cast<i32>(managed.system->GetTypeName().size()), managed.system->GetTypeName().data(), (after - before).ToSec_f64());
            PassControlsToOtherSystemsAndClear(managed.controlsToSendQueue, managed.system.get());
            PatchComponentAddedMessages(env.messageBuilder);
            PatchEntityRemovedArchetypes(env.messageBuilder);
            UpdateECSFromMessages(env.messageBuilder);
            PassMessagesToIndirectSystemsAndClear(env.messageBuilder, managed.system.get());
        }

        ExecuteIndirectSystem(*managed.system, managed.messageQueue, managed.controlsReceivedQueue, managed.controlsToSendQueue, env);

        ++managed.executedTimes;
    }

    ++pipeline.executionFrame;
}

void SystemsManagerST::ProcessMessagesAndClear(BaseIndirectSystem &system, ManagedIndirectSystem::MessageQueue &messageQueue, System::Environment &env)
{
	for (const auto &stream : messageQueue.entityAddedStreams)
	{
		system.ProcessMessages(env, stream);
	}
    for (const auto &stream : messageQueue.componentAddedStreams)
    {
        system.ProcessMessages(env, stream);
    }
	for (const auto &stream : messageQueue.componentChangedStreams)
	{
		system.ProcessMessages(env, stream);
	}
    for (const auto &stream : messageQueue.componentRemovedStreams)
    {
        system.ProcessMessages(env, stream);
    }
	for (const auto &stream : messageQueue.entityRemovedStreams)
	{
		system.ProcessMessages(env, stream);
	}

    messageQueue.clear();
}

void SystemsManagerST::ExecuteIndirectSystem(BaseIndirectSystem &system, ManagedIndirectSystem::MessageQueue &messageQueue, ControlsQueue &controlsReceivedQueue, ControlsQueue &controlsToSendQueue, System::Environment &env)
{
    ProcessControlsQueueAndClear(system, controlsReceivedQueue);

    IKeyController::ListenerHandle addToQueueHandle;
    if (env.keyController)
    {
        addToQueueHandle = env.keyController->OnControlAction(std::bind(&SendControlActionToQueue, std::ref(controlsToSendQueue), _1));
    }

    ProcessMessagesAndClear(system, messageQueue, env);

    system.Update(env);

    auto requested = system.RequestedComponents().writeAccess;

    for (const auto &[componentType, stream] : env.messageBuilder.ComponentChangedStreams()._data)
    {
        auto it = requested.find_if([componentType = componentType](const System::ComponentRequest &r) { return r.type == componentType; });
        ASSUME(it != requested.end()); // system changed component without requesting write access
    }

    PassControlsToOtherSystemsAndClear(controlsToSendQueue, &system);
    PatchComponentAddedMessages(env.messageBuilder);
    PatchEntityRemovedArchetypes(env.messageBuilder);
    UpdateECSFromMessages(env.messageBuilder);
    PassMessagesToIndirectSystemsAndClear(env.messageBuilder, &system);
}

void SystemsManagerST::ExecuteDirectSystem(BaseDirectSystem &system, ControlsQueue &controlsReceivedQueue, ControlsQueue &controlsToSendQueue, System::Environment &env)
{
    ProcessControlsQueueAndClear(system, controlsReceivedQueue);

    IKeyController::ListenerHandle addToQueueHandle;
    if (env.keyController)
    {
        addToQueueHandle = env.keyController->OnControlAction(std::bind(&SendControlActionToQueue, std::ref(controlsToSendQueue), _1));
    }

    auto &requested = system.RequestedComponents();

    uiw maxArgs = requested.withData.size() + (requested.entityIDIndex != nullopt) + (requested.environmentIndex != nullopt);

	_tempNonUniqueArgs.reserve(maxArgs);
    _tempArrayArgs.reserve(maxArgs);
    _tempArgs.reserve(maxArgs);

    const auto &archetypes = _archetypeReflector.FindMatchingArchetypes(reinterpret_cast<uiw>(&system));

    for (const Archetype &archetype : archetypes)
    {
        auto it = _archetypeGroups.find(archetype);
        ASSUME(it != _archetypeGroups.end());

        ASSUME(_tempNonUniqueArgs.capacity() >= maxArgs && _tempArgs.capacity() >= maxArgs && _tempArrayArgs.capacity() >= maxArgs); // any unexpected reallocation will break the program

        for (const auto &group : it->second)
        {
            if (group.get().entitiesCount == 0)
            {
                continue;
            }

			_tempNonUniqueArgs.clear();
            _tempArrayArgs.clear();
            _tempArgs.clear();

            for (const System::ComponentRequest &arg : requested.argumentPassingOrder)
            {
				ASSUME(arg.requirement == RequirementForComponent::OptionalWithData || arg.requirement == RequirementForComponent::RequiredWithData);

                ui32 index = 0;
                for (; index < group.get().uniqueTypedComponentsCount; ++index)
                {
                    if (group.get().components[index].type == arg.type)
                    {
                        break;
                    }
                }

                bool isFound = index < group.get().uniqueTypedComponentsCount;

                if (isFound)
                {
					const auto &component = group.get().components[index];
					ASSUME(Funcs::IsAligned(component.data.get(), component.alignmentOf));

					if (component.isUnique)
					{
						_tempArrayArgs.push_back({component.data.get(), group.get().entitiesCount});
						_tempArgs.push_back(&_tempArrayArgs.back());
					}
					else
					{
						NonUnique<byte> desc =
						{
							{component.data.get(), group.get().entitiesCount * component.stride},
							{component.ids.get(), group.get().entitiesCount * component.stride},
							component.stride
						};
						_tempNonUniqueArgs.push_back(desc);
						_tempArgs.push_back(&_tempNonUniqueArgs.back());
					}
                }
                else
                {
                    ASSUME(arg.requirement == RequirementForComponent::OptionalWithData); // should have failed the archetype test if there's no such component
                    _tempArgs.push_back(nullptr);
                }
            }

			auto insertIds = [this, &requested, &group]
			{
				if (requested.entityIDIndex)
				{
					_tempArrayArgs.push_back({reinterpret_cast<byte *>(group.get().entities.get()), group.get().entitiesCount});
					_tempArgs.insert(_tempArgs.begin() + *requested.entityIDIndex, &_tempArrayArgs.back());
				}
			};

			auto insertEnv = [this, &requested, &env]
			{
				if (requested.environmentIndex)
				{
					_tempArgs.insert(_tempArgs.begin() + *requested.environmentIndex, &env);
				}
			};

			if (requested.entityIDIndex < requested.environmentIndex)
			{
				insertIds();
				insertEnv();
			}
			else
			{
				insertEnv();
				insertIds();
			}

            ASSUME(_tempArgs.size() <= maxArgs);

            system.AcceptUntyped(_tempArgs.data());

			for (const System::ComponentRequest &arg : system.RequestedComponents().writeAccess)
			{
				ui32 index = 0;
				for (; index < group.get().uniqueTypedComponentsCount; ++index)
				{
					if (group.get().components[index].type == arg.type)
					{
						break;
					}
				}

                bool isFound = index < group.get().uniqueTypedComponentsCount;
                if (isFound == false)
                {
                    ASSUME(arg.requirement == RequirementForComponent::OptionalWithData); // should have failed the archetype test if there's no such component
                    continue;
                }

                // check if there're indirect systems that require this component
                auto isRequestedByIndirect = [this](TypeId type)
                {
                    for (auto &pipeline : _pipelines)
                    {
                        for (auto &managed : pipeline.indirectSystems)
                        {
                            auto otherReq = managed.system->RequestedComponents().withData;
                            for (auto &c : otherReq)
                            {
                                if (c.type == type)
                                {
                                    return true;
                                }
                            }
                        }
                    }
                    return false;
                };
                if (isRequestedByIndirect(arg.type) == false)
                {
                    continue;
                }

				auto &stored = group.get().components[index];

				SerializedComponent serialized;
				serialized.alignmentOf = stored.alignmentOf;
				serialized.isUnique = stored.isUnique;
				serialized.sizeOf = stored.sizeOf;
				serialized.type = stored.type;
                serialized.isTag = false;

				for (ui32 component = 0; component < group.get().entitiesCount; ++component)
				{
					EntityID entityID = group.get().entities[component];
					for (ui32 stride = 0; stride < stored.stride; ++stride)
					{
						serialized.data = stored.data.get() + stored.sizeOf * component * stored.stride + stride * stored.sizeOf;
						if (stored.isUnique == false)
						{
							serialized.id = stored.ids[component * stored.stride + stride];
                            ASSUME(serialized.id);
						}

						env.messageBuilder.ComponentChanged(entityID, serialized);
					}
				}
			}
        }
    }

    PassControlsToOtherSystemsAndClear(controlsToSendQueue, &system);
    PatchComponentAddedMessages(env.messageBuilder);
    PatchEntityRemovedArchetypes(env.messageBuilder);
    UpdateECSFromMessages(env.messageBuilder);
    PassMessagesToIndirectSystemsAndClear(env.messageBuilder, nullptr);
}

void SystemsManagerST::ProcessControlsQueueAndClear(System &system, ControlsQueue &controlsQueue)
{
    if (controlsQueue.size() == 0)
    {
        return;
    }

    ASSUME(system.GetKeyController());

    system.GetKeyController()->Dispatch(controlsQueue);

    controlsQueue.clear();
}

void SystemsManagerST::PassControlsToOtherSystemsAndClear(ControlsQueue &controlsQueue, System *systemToIgnore)
{
    if (controlsQueue.size() == 0)
    {
        return;
    }

    for (auto &pipeline : _pipelines)
    {
        for (auto &system : pipeline.directSystems)
        {
            if (system.system.get() != systemToIgnore && system.system->GetKeyController())
            {
                system.controlsReceivedQueue.Enqueue(controlsQueue.Actions());
            }
        }
        for (auto &system : pipeline.indirectSystems)
        {
            if (system.system.get() != systemToIgnore && system.system->GetKeyController())
            {
                system.controlsReceivedQueue.Enqueue(controlsQueue.Actions());
            }
        }
    }

    controlsQueue.clear();
}

void SystemsManagerST::PatchComponentAddedMessages(MessageBuilder &messageBuilder)
{
    for (auto &[componentType, stream] : messageBuilder.ComponentAddedStreams()._data)
    {
        for (auto &info : *stream)
        {
            auto it = _entitiesLocations.find(info.entityID);
            ASSUME(it != _entitiesLocations.end());

            const auto &group = *it->second.group;

            for (ui32 index = 0; index < group.uniqueTypedComponentsCount; ++index)
            {
                const auto &stored = group.components[index];

                SerializedComponent serialized;
                serialized.alignmentOf = stored.alignmentOf;
                serialized.isTag = false;
                serialized.isUnique = stored.isUnique;
                serialized.sizeOf = stored.sizeOf;
                serialized.type = stored.type;
                
                for (ui32 stride = 0; stride < stored.stride; ++stride)
                {
                    serialized.data = stored.data.get() + stored.sizeOf * it->second.index * stored.stride + stride * stored.sizeOf;
                    if (!serialized.isUnique)
                    {
                        serialized.id = stored.ids[it->second.index * stored.stride + stride];
                    }

                    info.cab.AddComponent(serialized);
                }
            }

            info.componentsData = move(info.cab._data);
            info.components = move(info.cab._components);
            info.added = info.components.front();
        }
    }
}

void SystemsManagerST::PatchEntityRemovedArchetypes(MessageBuilder &messageBuilder)
{
    for (EntityID id : messageBuilder.EntityRemovedNoArchetype())
    {
        auto it = _entitiesLocations.find(id);
        ASSUME(it != _entitiesLocations.end());
        messageBuilder.RemoveEntity(id, it->second.group->archetype.ToShort());
    }
}

void SystemsManagerST::UpdateECSFromMessages(MessageBuilder &messageBuilder)
{
    auto removeEntity = [this](ArchetypeGroup &group, ui32 index, optional<decltype(_entitiesLocations)::iterator> entityLocation)
    {
        --group.entitiesCount;
        uiw replaceIndex = group.entitiesCount;

        bool isReplaceWithLast = replaceIndex != index;

        if (isReplaceWithLast)
        {
            // copy entity id
            group.entities[index] = group.entities[replaceIndex];

            // copy components data and optionally components ids
            for (uiw componentIndex = 0; componentIndex < group.uniqueTypedComponentsCount; ++componentIndex)
            {
                auto &arr = group.components[componentIndex];
				byte *target = arr.data.get() + arr.sizeOf * index * arr.stride;
                const byte *source = arr.data.get() + arr.sizeOf * replaceIndex * arr.stride;
                uiw copySize = arr.sizeOf * arr.stride;
                MemOps::Copy(target, source, copySize);

                if (!arr.isUnique)
                {
                    ComponentID *idTarget = arr.ids.get() + index * arr.stride;
                    const ComponentID *idSource = arr.ids.get() + replaceIndex * arr.stride;
                    uiw idCopySize = sizeof(ComponentID) * arr.stride;
                    MemOps::Copy(idTarget, idSource, idCopySize);
                }
            }
        }

        if (entityLocation)
        {
            // remove deleted entity location
            _entitiesLocations.erase(*entityLocation);
        }

        if (isReplaceWithLast)
        {
            // patch replaced entity's index
            auto location = _entitiesLocations.find(group.entities[index]);
            ASSUME(location != _entitiesLocations.end());
            location->second.index = index;
        }
    };

    auto addOrRemoveComponent = [this, removeEntity](EntityID entityID, TypeId typeToRemove, ComponentID componentIDToRemove, optional<SerializedComponent> componentToAdd)
    {
        auto entityLocation = _entitiesLocations.find(entityID);
        ASSUME(entityLocation != _entitiesLocations.end()); // requested entity that doesn't exist

        auto[group, indexInGroup] = entityLocation->second;

		ASSUME(_tempComponents.empty());

		bool isFoundRemoveTarget = false;

        // collect the current components
        for (ui32 componentTypeIndex = 0; componentTypeIndex < group->uniqueTypedComponentsCount; ++componentTypeIndex)
        {
            auto &row = group->components[componentTypeIndex];
            for (ui32 nonUniqueIndex = 0; nonUniqueIndex < row.stride; ++nonUniqueIndex)
            {
                SerializedComponent serialized;
                if (row.isUnique == false)
                {
                    serialized.id = row.ids[indexInGroup * row.stride + nonUniqueIndex];
                    ASSUME(serialized.id);
                }
                serialized.type = row.type;
                if (serialized.id == componentIDToRemove && serialized.type == typeToRemove)
                {
					isFoundRemoveTarget = true;
                    continue;
                }
                serialized.alignmentOf = row.alignmentOf;
                serialized.isUnique = row.isUnique;
                serialized.isTag = false;
                serialized.sizeOf = row.sizeOf;
                serialized.data = row.data.get() + row.sizeOf * indexInGroup * row.stride + row.sizeOf * nonUniqueIndex;
                _tempComponents.push_back(serialized);
            }
        }

        if (componentToAdd)
        {
            _tempComponents.push_back(*componentToAdd);
        }

        for (ui32 tagIndex = 0; tagIndex < group->tagsCount; ++tagIndex)
        {
            if (typeToRemove == group->tags[tagIndex])
            {
				isFoundRemoveTarget = true;
                continue;
            }

            SerializedComponent serialized;
            serialized.isTag = true;
            serialized.isUnique = true;
            serialized.type = group->tags[tagIndex];
            _tempComponents.push_back(serialized);
        }

		ASSUME(isFoundRemoveTarget || typeToRemove == TypeId{}); // trying to remove component that doesn't exist

        ArchetypeGroup *newGroup;
        ArchetypeFull archetype = ComputeArchetype(ToArray(_tempComponents));
        auto groupSearch = _archetypeGroupsFull.find(archetype);
        if (groupSearch == _archetypeGroupsFull.end())
        {
            newGroup = &AddNewArchetypeGroup(archetype, ToArray(_tempComponents));
        }
        else
        {
            ASSUME(&groupSearch->second != group);
            newGroup = &groupSearch->second;
        }

		#ifdef DEBUG
			for (auto index = 0; index < newGroup->uniqueTypedComponentsCount; ++index)
			{
				ASSUME(std::find_if(_tempComponents.begin(), _tempComponents.end(), [type = newGroup->components[index].type](const SerializedComponent &stored) { return type == stored.type; }) != _tempComponents.end());
			}
			for (auto index = 0; index < newGroup->tagsCount; ++index)
			{
				ASSUME(std::find_if(_tempComponents.begin(), _tempComponents.end(), [type = newGroup->tags[index]](const SerializedComponent &stored) { return type == stored.type; }) != _tempComponents.end());
			}
			for (const SerializedComponent &stored : _tempComponents)
			{
				if (stored.isTag)
				{
					ASSUME(std::find_if(newGroup->tags.get(), newGroup->tags.get() + newGroup->tagsCount, [type = stored.type](const auto &c) { return c == type; }) != newGroup->tags.get() + newGroup->tagsCount);
				}
				else
				{
					ASSUME(std::find_if(newGroup->components.get(), newGroup->components.get() + newGroup->uniqueTypedComponentsCount, [type = stored.type](const auto &c) { return c.type == type; }) != newGroup->components.get() + newGroup->uniqueTypedComponentsCount);
				}
			}
		#endif

		ASSUME(newGroup != group);

        AddEntityToArchetypeGroup(archetype, *newGroup, entityID, ToArray(_tempComponents), nullptr);

        removeEntity(*group, indexInGroup, nullopt);

		_tempComponents.clear();
    };

    // update the ECS using the received messages

    for (auto &[streamArchetype, stream] : messageBuilder.EntityAddedStreams()._data)
    {
        for (auto &entity : *stream)
        {
            AssignComponentIDs(ToArray(entity.components), _componentIdGenerator);

            ArchetypeFull archetype = ComputeArchetype(ToArray(entity.components));

            ArchetypeGroup *group;
            auto groupSearch = _archetypeGroupsFull.find(archetype);
            if (groupSearch == _archetypeGroupsFull.end())
            {
                group = &AddNewArchetypeGroup(archetype, ToArray(entity.components));
            }
            else
            {
                group = &groupSearch->second;
            }

            AddEntityToArchetypeGroup(archetype, *group, entity.entityID, ToArray(entity.components), nullptr);
        }
    }

    for (auto &[componentType, stream] : messageBuilder.ComponentAddedStreams()._data)
    {
        for (const auto &info : *stream)
        {
            addOrRemoveComponent(info.entityID, {}, {}, info.added);
        }
    }

    for (const auto &[componentType, descWithStream] : messageBuilder.ComponentChangedStreams()._data)
    {
		const auto &[desc, stream] = descWithStream;

		ArchetypeGroup *prevGroup = nullptr;
		if (_archetypeGroups.size() && _archetypeGroups.begin()->second.size())
		{
			// the only case when there're no groups is when there're no entities,
			// in that case any component changed message is an error
			prevGroup = &_archetypeGroups.begin()->second.data()->get();
		}
		uiw prevEntityIndex = uiw_max;

		for (uiw index = 0, size = stream->entityIds.size(); index < size; ++index)
        {
			EntityID entityID = stream->entityIds[index];

            ASSUME(!desc.isTag); // tag components cannot be changed
			ASSUME(entityID);

			ArchetypeGroup *group = prevGroup;
			uiw entityIndex = prevEntityIndex + 1;

			if (entityID != prevGroup->entities[entityIndex])
			{
				auto entityLocation = _entitiesLocations.find(entityID);
				ASSUME(entityLocation != _entitiesLocations.end());

				group = entityLocation->second.group;
				entityIndex = entityLocation->second.index;
			}

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

            ASSUME(desc.alignmentOf == componentArray.alignmentOf);
            ASSUME(desc.isUnique == componentArray.isUnique);
            ASSUME(desc.sizeOf == componentArray.sizeOf);
            ASSUME(desc.type == componentArray.type);

            uiw offset = 0;
            if (!desc.isUnique)
            {
                for (; ; ++offset)
                {
                    ASSUME(offset < componentArray.stride);
                    if (stream->componentIds[index] == componentArray.ids[entityIndex * componentArray.stride + offset])
                    {
                        break;
                    }
                }
            }

            MemOps::Copy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * entityIndex + componentArray.sizeOf * offset, stream->data.get() + index * desc.sizeOf, desc.sizeOf);

			prevGroup = group;
			prevEntityIndex = entityIndex;
        }
    }

    for (const auto &[componentType, stream] : messageBuilder.ComponentRemovedStreams()._data)
    {
		for (uiw index = 0, size = stream->entityIds.size(); index < size; ++index)
		{
			ComponentID componentID;
			if (stream->componentIds.size())
			{
				componentID = stream->componentIds[index];
			}
			addOrRemoveComponent(stream->entityIds[index], componentType, componentID, nullopt);
		}
    }

    for (const auto &[streamArchetype, stream] : messageBuilder.EntityRemovedStreams()._data)
    {
        for (auto &entity : *stream)
        {
            auto entityLocation = _entitiesLocations.find(entity);
            ASSUME(entityLocation != _entitiesLocations.end());
            removeEntity(*entityLocation->second.group, entityLocation->second.index, entityLocation);
        }
    }
}

void SystemsManagerST::PassMessagesToIndirectSystemsAndClear(MessageBuilder &messageBuilder, System *systemToIgnore)
{
    for (auto &[streamArchetype, streamPointer] : messageBuilder.EntityAddedStreams()._data)
    {
        auto reflected = _archetypeReflector.Reflect(streamArchetype);
        auto stream = MessageStreamEntityAdded(streamArchetype, streamPointer, messageBuilder.SourceName());

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != systemToIgnore)
                {
                    if (ArchetypeReflector::Satisfies(reflected, managed.system->RequestedComponents().archetypeDefiningInfoOnly))
                    {
                        managed.messageQueue.entityAddedStreams.emplace_back(stream);
                    }
                }
            }
        }
    }

    for (const auto &[componentType, streamPointer] : messageBuilder.ComponentAddedStreams()._data)
    {
        auto stream = MessageStreamComponentAdded(componentType, streamPointer, messageBuilder.SourceName());

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != systemToIgnore)
                {
                    auto requested = managed.system->RequestedComponents();
                    auto searchPredicate = [componentType = componentType](const System::ComponentRequest &stored) { return componentType == stored.type; };

                    if (requested.subtractive.find_if(searchPredicate) != requested.subtractive.end())
                    {
                        continue;
                    }

                    if (requested.required.empty() ||
                        requested.requiredOrOptional.count_if(searchPredicate))
                    {
                        managed.messageQueue.componentAddedStreams.emplace_back(stream);
                    }
                }
            }
        }
    }

    for (const auto &[componentType, streamPointer] : messageBuilder.ComponentChangedStreams()._data)
    {
        auto stream = MessageStreamComponentChanged(streamPointer.second, streamPointer.first, messageBuilder.SourceName());

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != systemToIgnore)
                {
                    auto requested = managed.system->RequestedComponents();
                    auto searchPredicate = [componentType = componentType](const System::ComponentRequest &stored) { return componentType == stored.type; };

                    if (requested.subtractive.find_if(searchPredicate) != requested.subtractive.end())
                    {
                        continue;
                    }

                    if (requested.required.empty() ||
                        requested.requiredOrOptional.count_if(searchPredicate))
                    {
                        managed.messageQueue.componentChangedStreams.emplace_back(stream);
                    }
                }
            }
        }
    }

    for (const auto &[componentType, streamPointer] : messageBuilder.ComponentRemovedStreams()._data)
    {
        auto stream = MessageStreamComponentRemoved(componentType, streamPointer, messageBuilder.SourceName());

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != systemToIgnore)
                {
                    auto requested = managed.system->RequestedComponents();
                    auto searchPredicate = [componentType = componentType](const System::ComponentRequest &stored) { return componentType == stored.type; };

                    if (requested.subtractive.find_if(searchPredicate) != requested.subtractive.end())
                    {
                        continue;
                    }

                    if (requested.required.empty() ||
                        requested.requiredOrOptional.count_if(searchPredicate))
                    {
                        managed.messageQueue.componentRemovedStreams.emplace_back(stream);
                    }
                }
            }
        }
    }

    for (const auto &[streamArchetype, streamPointer] : messageBuilder.EntityRemovedStreams()._data)
    {
        auto reflected = _archetypeReflector.Reflect(streamArchetype);
        auto stream = MessageStreamEntityRemoved(streamArchetype, streamPointer, messageBuilder.SourceName());

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != systemToIgnore)
                {
                    if (ArchetypeReflector::Satisfies(reflected, managed.system->RequestedComponents().archetypeDefiningInfoOnly))
                    {
                        managed.messageQueue.entityRemovedStreams.emplace_back(stream);
                    }
                }
            }
        }
    }

    messageBuilder.Clear();
}

bool SystemsManagerST::SendControlActionToQueue(ControlsQueue &controlsQueue, const ControlAction &action)
{
    controlsQueue.push_back(action);
    return false;
}

void SystemsManagerST::ManagedIndirectSystem::MessageQueue::clear()
{
    entityAddedStreams.clear();
    componentAddedStreams.clear();
    componentChangedStreams.clear();
    componentRemovedStreams.clear();
    entityRemovedStreams.clear();
}

bool SystemsManagerST::ManagedIndirectSystem::MessageQueue::empty() const
{
    return
        entityAddedStreams.empty() &&
        componentAddedStreams.empty() &&
        componentChangedStreams.empty() &&
        componentRemovedStreams.empty() &&
        entityRemovedStreams.empty();
}