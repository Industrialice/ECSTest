#include "PreHeader.hpp"
#include "SystemsManagerST.hpp"

namespace ECSTest
{
    class ECSEntitiesST : public EntitiesStream
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
        for (ui32 index = 0; index < (ui32)_pipelines.size(); ++index)
        {
            if (_pipelines[index].executionStep == executionStep)
            {
                group.index = index;
                return group;
            }
        }
    }
    group.index = (ui32)_pipelines.size();
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
    info.directSystems = (ui32)pipelineData.directSystems.size();
    info.indirectSystems = (ui32)pipelineData.indirectSystems.size();
    info.executionStep = pipelineData.executionStep;

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
		uiw requiredCount = requestedComponents.required.size();
        uiw requiredWithDataCount = requestedComponents.requiredWithData.size();
		uiw optionalCount = requestedComponents.optional.size();

		if (requiredCount == 0 && requiredWithDataCount == 0 && optionalCount == 0)
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
				if (existingSystem.system->Type() == system->Type())
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
				if (existingSystem.system->Type() == system->Type())
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

	_archetypeReflector.StartTrackingMatchingArchetypes((uiw)system.get(), requestedComponents.archetypeDefining);

	if (isDirectSystem)
	{
		ManagedDirectSystem direct;
		direct.executedAt = pipelineData.executionFrame - 1;
		direct.system.reset(system.release()->AsDirectSystem());
		pipelineData.directSystems.emplace_back(move(direct));
	}
	else
	{
		ManagedIndirectSystem indirect;
		indirect.executedAt = pipelineData.executionFrame - 1;
		indirect.system.reset(system.release()->AsIndirectSystem());
		pipelineData.indirectSystems.emplace_back(move(indirect));
	}
}

void SystemsManagerST::Unregister(StableTypeId systemType)
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

    _logger->Message(LogLevels::Error, selfName, "System not found");
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
		ASSUME(component.isUnique || component.id.IsValid());
	}
	return ArchetypeFull::Create<SerializedComponent, &SerializedComponent::type, &SerializedComponent::id>(components);
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

void SystemsManagerST::Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, vector<unique_ptr<EntitiesStream>> &&streams)
{
	ASSUME(_entitiesLocations.empty());

	if (workers.size())
	{
        _logger->Message(LogLevels::Warning, selfName, "Workers aren't used by a single threaded systems manager");
		workers.clear();
	}

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

shared_ptr<EntitiesStream> SystemsManagerST::StreamOut() const
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

	vector<StableTypeId> uniqueTypes;
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

	group.uniqueTypedComponentsCount = (ui16)uniqueTypes.size();
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

	group.reservedCount = 8; // by default initialize enough space for 8 entities

	for (ui16 index = 0; index < group.uniqueTypedComponentsCount; ++index)
	{
		auto &componentArray = group.components[index];

		// allocate memory
		ASSUME(componentArray.sizeOf > 0 && componentArray.stride > 0 && group.reservedCount > 0 && componentArray.alignmentOf > 0);
		componentArray.data.reset((ui8 *)_aligned_malloc(componentArray.sizeOf * componentArray.stride * group.reservedCount, componentArray.alignmentOf));

		if (!componentArray.isUnique)
		{
			uiw allocSize = sizeof(ComponentID) * componentArray.stride * group.reservedCount;
			componentArray.ids.reset((ComponentID *)malloc(allocSize));
			memset(componentArray.ids.get(), ComponentID::invalidId, allocSize);
		}
	}

	group.entities.reset((EntityID *)malloc(sizeof(EntityID) * group.reservedCount));
    
    // add tag components
    vector<StableTypeId> tagTypes;
    for (const auto &component : components)
    {
        if (component.isTag)
        {
            tagTypes.push_back(component.type);
        }
    }
    group.tagsCount = (ui16)tagTypes.size();
    group.tags = make_unique<StableTypeId[]>(group.tagsCount);
    std::copy(tagTypes.begin(), tagTypes.end(), group.tags.get());

	// also add that archetype to the library
    uniqueTypes.insert(uniqueTypes.end(), group.tags.get(), group.tags.get() + group.tagsCount);
    std::sort(uniqueTypes.begin(), uniqueTypes.end());
	_archetypeReflector.AddToLibrary(archetype.ToShort(), move(uniqueTypes));

	return group;
}

void SystemsManagerST::AddEntityToArchetypeGroup(const ArchetypeFull &archetype, ArchetypeGroup &group, EntityID entityId, Array<const SerializedComponent> components, MessageBuilder *messageBuilder)
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
				ComponentID *oldUPtr = componentArray.ids.release();
				ComponentID *newUPtr = (ComponentID *)realloc(oldUPtr, sizeof(ComponentID) * componentArray.stride * group.reservedCount);
				componentArray.ids.reset(newUPtr);
				memset(newUPtr + componentArray.stride * group.entitiesCount, ComponentID::invalidId, (group.reservedCount - group.entitiesCount) * sizeof(ComponentID) * componentArray.stride);
			}
		}

		EntityID *oldPtr = group.entities.release();
		EntityID *newPtr = (EntityID *)realloc(oldPtr, sizeof(EntityID) * group.reservedCount);
		group.entities.reset(newPtr);
	}

	optional<std::reference_wrapper<MessageBuilder::ComponentArrayBuilder>> componentBuilder;
	if (messageBuilder)
	{
		componentBuilder = messageBuilder->AddEntity(entityId);
	}

    uiw tagsCount = 0;

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
                ASSUME(component.id.IsValid());

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

            memcpy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * group.entitiesCount + offset * componentArray.sizeOf, component.data, componentArray.sizeOf);
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

void SystemsManagerST::StartScheduler(vector<unique_ptr<EntitiesStream>> &streams)
{
	MessageBuilder messageBulder;
    messageBulder.SourceName("Initial Streaming");
	vector<SerializedComponent> serialized;

	for (auto &stream : streams)
	{
		while (const auto &entity = stream->Next())
		{
			StreamedToSerialized(entity->components, serialized);
			AssignComponentIDs(ToArray(serialized), _componentIdGenerator);
			ArchetypeFull entityArchetype = ComputeArchetype(ToArray(serialized));
			ArchetypeGroup &archetypeGroup = FindArchetypeGroup(entityArchetype, ToArray(serialized));
			AddEntityToArchetypeGroup(entityArchetype, archetypeGroup, entity->entityId, ToArray(serialized), &messageBulder);
		}
	}

	auto &entityAddedStreams = messageBulder.EntityAddedStreams();
	for (auto &[archetype, messages] : entityAddedStreams._data)
	{
		auto reflected = _archetypeReflector.Reflect(archetype);
		MessageStreamEntityAdded stream = {archetype, messages, messageBulder.SourceName()};
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
				ProcessMessages(*managed.system, managed.messageQueue);
			}
            managed.messageQueue.clear();
		}
	}

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
}

void SystemsManagerST::SchedulerLoop()
{
    auto updateTimes = [this]() -> TimeMoment
    {
        auto currentTime = TimeMoment::Now();
        _timeSinceStart += (currentTime - _currentTime);
        _currentTime = currentTime;
        return currentTime;
    };

    uiw repeatedIndex = uiw_max;
    ui32 repeadedCount = 0;

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

		TimeDifference timeSinceLastFrame = 0_s;
		if (pipeline.lastExecutedTime.HasValue())
		{
			timeSinceLastFrame = TimeMoment::Now() - pipeline.lastExecutedTime;
		}

		System::Environment env =
		{
			timeSinceLastFrame.ToSec(),
			pipeline.executionFrame,
			_timeSinceStart,
			_entityIdGenerator,
            _componentIdGenerator,
            MessageBuilder(),
			LoggerWrapper(&*_logger, "")
		};

        ExecutePipeline(pipeline, env);

        auto currentTime = updateTimes();

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

    updateTimes();
    _timeSinceStartAtomic = _timeSinceStart;
}

void SystemsManagerST::ExecutePipeline(PipelineData &pipeline, System::Environment &env)
{
    for (auto &managed : pipeline.directSystems)
    {
        env.messageBuilder.SourceName(managed.system->Type().Name());

        managed.executedAt = pipeline.executionFrame;

        if (managed.executedTimes == 0)
        {
            managed.system->OnCreate(env);
            UpdateECSFromMessages(env.messageBuilder);
            PassMessagesToIndirectSystems(env.messageBuilder, nullptr);
			env.messageBuilder.SourceName(managed.system->Type().Name()); // the builder was reset, set the name again
        }

        env.logger._name = managed.system->Name();
        ExecuteDirectSystem(*managed.system, env);

        ++managed.executedTimes;
    }

    for (auto &managed : pipeline.indirectSystems)
    {
        env.messageBuilder.SourceName(managed.system->Type().Name());

        managed.executedAt = pipeline.executionFrame;

        if (managed.executedTimes == 0)
        {
            managed.system->OnCreate(env);
            UpdateECSFromMessages(env.messageBuilder);
            PassMessagesToIndirectSystems(env.messageBuilder, managed.system.get());
			env.messageBuilder.SourceName(managed.system->Type().Name()); // the builder was reset, set the name again
        }

        env.logger._name = managed.system->Name();
        ExecuteIndirectSystem(*managed.system, managed.messageQueue, env);

        ++managed.executedTimes;
    }

    ++pipeline.executionFrame;
}

void SystemsManagerST::ProcessMessages(IndirectSystem &system, const ManagedIndirectSystem::MessageQueue &messageQueue)
{
	for (const auto &stream : messageQueue.entityAddedStreams)
	{
		system.ProcessMessages(stream);
	}
    for (const auto &stream : messageQueue.componentAddedStreams)
    {
        system.ProcessMessages(stream);
    }
	for (const auto &stream : messageQueue.componentChangedStreams)
	{
		system.ProcessMessages(stream);
	}
    for (const auto &stream : messageQueue.componentRemovedStreams)
    {
        system.ProcessMessages(stream);
    }
	for (const auto &stream : messageQueue.entityRemovedStreams)
	{
		system.ProcessMessages(stream);
	}
}

void SystemsManagerST::ExecuteIndirectSystem(IndirectSystem &system, ManagedIndirectSystem::MessageQueue &messageQueue, System::Environment &env)
{
    ProcessMessages(system, messageQueue);
    messageQueue.clear();

    system.Update(env);

    auto requested = system.RequestedComponents().writeAccess;

    for (const auto &[componentType, stream] : env.messageBuilder.ComponentChangedStreams()._data)
    {
        auto it = requested.find_if([componentType](const System::RequestedComponent &r) { return r.type == componentType; });
        ASSUME(it != requested.end()); // system changed component without requesting write access
    }

    UpdateECSFromMessages(env.messageBuilder);
    PassMessagesToIndirectSystems(env.messageBuilder, &system);
}

void SystemsManagerST::ExecuteDirectSystem(DirectSystem &system, System::Environment &env)
{
    auto &reqested = system.RequestedComponents();

    uiw maxArgs = reqested.requiredWithData.size() + reqested.optional.size() + (reqested.idsArgumentNumber != nullopt) + reqested.required.size();

	_tempNonUniqueArgs.reserve(maxArgs);
    _tempArrayArgs.reserve(maxArgs);
    _tempArgs.reserve(maxArgs);

    const auto &archetypes = _archetypeReflector.FindMatchingArchetypes((uiw)&system);

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

            for (const System::RequestedComponent &arg : reqested.allOriginalOrder)
            {
                if (arg.requirement == RequirementForComponent::Required)
                {
                    _tempArgs.push_back(nullptr);
                    continue; // already passed the archetype check
                }

                ui32 index = 0;
                for (; index < group.get().uniqueTypedComponentsCount; ++index)
                {
                    if (group.get().components[index].type == arg.type)
                    {
                        break;
                    }
                }

                bool isFound = index < group.get().uniqueTypedComponentsCount;

                if (arg.requirement == RequirementForComponent::Subtractive)
                {
                    ASSUME(isFound == false);
                    continue;
                }

                if (isFound)
                {
					if (group.get().components[index].isUnique)
					{
						_tempArrayArgs.push_back({group.get().components[index].data.get(), group.get().entitiesCount});
						_tempArgs.push_back(&_tempArrayArgs.back());
					}
					else
					{
						NonUnique<ui8> desc = 
						{
							{group.get().components[index].data.get(), group.get().entitiesCount * group.get().components[index].stride},
							{group.get().components[index].ids.get(), group.get().entitiesCount * group.get().components[index].stride},
							group.get().components[index].stride
						};
						_tempNonUniqueArgs.push_back(desc);
						_tempArgs.push_back(&_tempNonUniqueArgs.back());
					}
                }
                else
                {
                    ASSUME(arg.requirement == RequirementForComponent::Optional);
                    _tempArgs.push_back(nullptr);
                }
            }

            if (reqested.idsArgumentNumber)
            {
                _tempArrayArgs.insert(_tempArrayArgs.begin() + *reqested.idsArgumentNumber, {(ui8 *)group.get().entities.get(), group.get().entitiesCount});
                _tempArgs.insert(_tempArgs.begin() + *reqested.idsArgumentNumber, &_tempArrayArgs.back());
            }

            ASSUME(_tempArgs.size() <= maxArgs);

            system.Accept(env, _tempArgs.data());

			for (const System::RequestedComponent &arg : system.RequestedComponents().writeAccess)
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
                    ASSUME(arg.requirement == RequirementForComponent::Optional);
                    continue;
                }

                // check if there're indirect systems that require this component
                auto isRequestedByIndirect = [this](StableTypeId type)
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
                            ASSUME(serialized.id.IsValid());
						}

						env.messageBuilder.ComponentChanged(entityID, serialized);
					}
				}
			}
        }
    }

    UpdateECSFromMessages(env.messageBuilder);
    PassMessagesToIndirectSystems(env.messageBuilder, nullptr);
}

void SystemsManagerST::UpdateECSFromMessages(MessageBuilder &messageBuilder)
{
    for (EntityID id : messageBuilder.EntityRemovedNoArchetype())
    {
        auto it = _entitiesLocations.find(id);
        ASSUME(it != _entitiesLocations.end());
        messageBuilder.RemoveEntity(id, it->second.group->archetype.ToShort());
    }

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
                void *target = arr.data.get() + arr.sizeOf * index * arr.stride;
                void *source = arr.data.get() + arr.sizeOf * replaceIndex * arr.stride;
                uiw copySize = arr.sizeOf * arr.stride;
                memcpy(target, source, copySize);

                if (!arr.isUnique)
                {
                    ComponentID *idTarget = arr.ids.get() + index * arr.stride;
                    ComponentID *idSource = arr.ids.get() + replaceIndex * arr.stride;
                    uiw idCopySize = sizeof(ComponentID) * arr.stride;
                    memcpy(idTarget, idSource, idCopySize);
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

    auto addOrRemoveComponent = [this, removeEntity](EntityID entityID, StableTypeId typeToRemove, ComponentID componentIDToRemove, optional<SerializedComponent> componentToAdd)
    {
        auto entityLocation = _entitiesLocations.find(entityID);
        ASSUME(entityLocation != _entitiesLocations.end());

        auto[group, indexInGroup] = entityLocation->second;

        _tempComponents.clear();

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
                    ASSUME(serialized.id.IsValid());
                }
                serialized.type = row.type;
                if (serialized.id == componentIDToRemove && serialized.type == typeToRemove)
                {
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
                continue;
            }

            SerializedComponent serialized;
            serialized.isTag = true;
            serialized.isUnique = true;
            serialized.type = group->tags[tagIndex];
            _tempComponents.push_back(serialized);
        }

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

        AddEntityToArchetypeGroup(archetype, *newGroup, entityID, ToArray(_tempComponents), nullptr);

        removeEntity(*group, indexInGroup, nullopt);
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
        for (const auto &info : stream->infos)
        {
            addOrRemoveComponent(info.entityID, {}, {}, info.component);
        }
    }

    for (const auto &[componentType, stream] : messageBuilder.ComponentChangedStreams()._data)
    {
        for (const auto &info : stream->infos)
        {
            ASSUME(!info.component.isTag); // tag components cannot be changed

            auto entityLocation = _entitiesLocations.find(info.entityID);
            ASSUME(entityLocation != _entitiesLocations.end());

            auto &[group, entityIndex] = entityLocation->second;

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

            memcpy(componentArray.data.get() + componentArray.sizeOf * componentArray.stride * entityIndex + componentArray.sizeOf * offset, info.component.data, info.component.sizeOf);
        }
    }

    for (const auto &[componentType, stream] : messageBuilder.ComponentRemovedStreams()._data)
    {
        for (const auto &[entityID, componentID] : *stream)
        {
            addOrRemoveComponent(entityID, componentType, componentID, nullopt);
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

void SystemsManagerST::PassMessagesToIndirectSystems(MessageBuilder &messageBuilder, System *systemToIgnore)
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
                    if (ArchetypeReflector::Satisfies(reflected, managed.system->RequestedComponents().archetypeDefining))
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
                    auto searchPredicate = [componentType](const System::RequestedComponent &stored) { return componentType == stored.type; };

                    if (requested.subtractive.find_if(searchPredicate) != requested.subtractive.end())
                    {
                        continue;
                    }

                    if (requested.requiredWithData.empty() ||
                        requested.requiredWithData.find_if(searchPredicate) != requested.requiredWithData.end() ||
                        requested.optional.find_if(searchPredicate) != requested.optional.end())
                    {
                        managed.messageQueue.componentAddedStreams.emplace_back(stream);
                    }
                }
            }
        }
    }

    for (const auto &[componentType, streamPointer] : messageBuilder.ComponentChangedStreams()._data)
    {
        auto stream = MessageStreamComponentChanged(componentType, streamPointer, messageBuilder.SourceName());

        for (auto &pipeline : _pipelines)
        {
            for (auto &managed : pipeline.indirectSystems)
            {
                if (managed.system.get() != systemToIgnore)
                {
                    auto requested = managed.system->RequestedComponents();
                    auto searchPredicate = [componentType](const System::RequestedComponent &stored) { return componentType == stored.type; };

                    if (requested.subtractive.find_if(searchPredicate) != requested.subtractive.end())
                    {
                        continue;
                    }

                    if (requested.requiredWithData.empty() ||
                        requested.requiredWithData.find_if(searchPredicate) != requested.requiredWithData.end() ||
                        requested.optional.find_if(searchPredicate) != requested.optional.end())
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
                    auto searchPredicate = [componentType](const System::RequestedComponent &stored) { return componentType == stored.type; };

                    if (requested.subtractive.find_if(searchPredicate) != requested.subtractive.end())
                    {
                        continue;
                    }

                    if (requested.requiredWithData.empty() ||
                        requested.requiredWithData.find_if(searchPredicate) != requested.requiredWithData.end() ||
                        requested.optional.find_if(searchPredicate) != requested.optional.end())
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
                    if (ArchetypeReflector::Satisfies(reflected, managed.system->RequestedComponents().archetypeDefining))
                    {
                        managed.messageQueue.entityRemovedStreams.emplace_back(stream);
                    }
                }
            }
        }
    }

    Funcs::Reinitialize(messageBuilder);
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