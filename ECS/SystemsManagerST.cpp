#include "PreHeader.hpp"
#include "SystemsManagerST.hpp"

using namespace ECSTest;

shared_ptr<SystemsManagerST> SystemsManagerST::New()
{
    struct Inherited : public SystemsManagerST
    {
        Inherited() : SystemsManagerST()
        {}
    };
    return make_shared<Inherited>();
}

auto SystemsManagerST::CreatePipelineGroup(optional<ui32> stepMicroSeconds, bool isMergeIfSuchPipelineExists) -> PipelineGroup
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

void SystemsManagerST::Register(unique_ptr<System> system, PipelineGroup pipelineGroup)
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

void SystemsManagerST::Unregister(StableTypeId systemType)
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
		ASSUME(component.isUnique || component.id.ID() != 0);
	}
	return ArchetypeFull::Create<SerializedComponent, &SerializedComponent::type, &SerializedComponent::id>(components);
}

static void AssignComponentIDs(Array<SerializedComponent> components, ui32 &lastComponentId)
{
	for (auto &component : components)
	{
		if (!component.isUnique)
		{
			component.id = ++lastComponentId;
		}
		else
		{
			ASSUME(component.id.ID() == 0);
		}
	}
}

void SystemsManagerST::Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, vector<unique_ptr<EntitiesStream>> &&streams)
{
	ASSUME(_entitiesLocations.empty());

	if (workers.size())
	{
		SOFTBREAK;
		workers.clear();
	}

	_idGenerator = move(idGenerator);

	_isStoppingExecution = false;
	_isPausedExecution = false;
	_isSchedulerPaused = false;

	_schedulerThread = std::thread([this, &streams] { StartScheduler(move(streams)); });
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
	_lastComponentId = 0;
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
	NOIMPL;
	return {};
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
			uiw allocSize = sizeof(ComponentID) * componentArray.stride * group.reservedCount;
			componentArray.ids.reset((ComponentID *)malloc(allocSize));
			memset(componentArray.ids.get(), 0x0, allocSize);
		}
	}

	group.entities.reset((EntityID *)malloc(sizeof(EntityID) * group.reservedCount));

	// also add that archetype to the library
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
				memset(newUPtr + componentArray.stride * group.entitiesCount, 0x0, (group.reservedCount - group.entitiesCount) * sizeof(ComponentID) * componentArray.stride);
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
				if (componentArray.ids[componentArray.stride * group.entitiesCount + offset].IsValid() == false)
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

	_entitiesLocations[entityId] = {&group, group.entitiesCount};

	++group.entitiesCount;
}

void SystemsManagerST::StartScheduler(vector<unique_ptr<EntitiesStream>> &&streams)
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
				ProcessMessages(*managed.system, managed.messageQueue);
			}
		}
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
}

void SystemsManagerST::SchedulerLoop()
{
	for (auto &pipeline : _pipelines)
	{
		ExecutePipeline(pipeline);
	}
}

void SystemsManagerST::ExecutePipeline(Pipeline &pipeline)
{
}

void SystemsManagerST::CalculateGroupsToExectute(const System *system, vector<std::reference_wrapper<ArchetypeGroup>> &groups)
{
	groups.clear();

	const auto &archetypes = _archetypeReflector.FindMatchingArchetypes((uiw)system);

	for (Archetype archetype : archetypes)
	{
		auto it = _archetypeGroups.find(archetype);
		ASSUME(it != _archetypeGroups.end());
		groups.insert(groups.end(), it->second.begin(), it->second.end());
	}
}

void SystemsManagerST::ProcessMessages(IndirectSystem &system, const ManagedIndirectSystem::MessageQueue &messageQueue)
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
