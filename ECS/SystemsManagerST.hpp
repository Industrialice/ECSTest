#pragma once

#include "SystemsManager.hpp"

namespace ECSTest
{
	class SystemsManagerST : public SystemsManager, public std::enable_shared_from_this<SystemsManagerST>
	{
		friend class ECSEntities;

	protected:
		~SystemsManagerST() = default;
		SystemsManagerST() = default;

	public:
		static shared_ptr<SystemsManagerST> New();

		[[nodiscard]] virtual PipelineGroup CreatePipelineGroup(optional<ui32> stepMicroSeconds, bool isMergeIfSuchPipelineExists) override;
		virtual void Register(unique_ptr<System> system, PipelineGroup pipelineGroup) override;
		virtual void Unregister(StableTypeId systemType) override;
		virtual void Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, vector<unique_ptr<EntitiesStream>> &&streams) override;
		virtual void Pause(bool isWaitForStop) override; // you can call it multiple times, for example first time as Pause(false), and then as Pause(true) to wait for paused
		virtual void Resume() override;
		virtual void Stop(bool isWaitForStop) override;
		[[nodiscard]] virtual bool IsRunning() const override;
		[[nodiscard]] virtual bool IsPaused() const override;
		//virtual void StreamIn(vector<unique_ptr<EntitiesStream>> &&streams) override;
		[[nodiscard]] virtual shared_ptr<EntitiesStream> StreamOut() const override; // the manager must be paused

	private:
		struct ArchetypeGroup
		{
			struct ComponentArray
			{
				StableTypeId type{}; // of each component
				ui16 stride{}; // Components of that type per entity, 1 if there's only one. Any access to a particular component must be performed using index * stride
				ui16 sizeOf{}; // of each component
				ui16 alignmentOf{}; // of each component
				unique_ptr<ui8[], AlignedMallocDeleter> data{}; // each component can be safely casted into class Component
				unique_ptr<ComponentID[], MallocDeleter> ids{}; // ComponentID, used only for components that allow multiple components of that type to be attached to an entity
				bool isUnique{}; // indicates whether other components of the same type can be attached to an entity
			};

			unique_ptr<ComponentArray[]> components{}; // essentially a 2D array where rows count = uniqueTypedComponentsCount, columns count is computed per row as entitiesCount * stride
			unique_ptr<EntityID[]> entities{};
			ui16 uniqueTypedComponentsCount{};
			ui16 reservedCount{}; // final reserved count is computed as reservedCount * stride
			ui32 entitiesCount{};
			ArchetypeFull archetype; // group's archetype
		};

		struct EntityLocation
		{
			ArchetypeGroup *group{};
			ui32 index{};
		};

		struct ManagedSystem
		{
			ui32 executedAt{}; // last executed frame, gets set to Pipeline::executionFrame at first execution attempt on a new frame
			vector<std::reference_wrapper<ArchetypeGroup>> groupsToExecute{}; // group still left to be executed for the current frame
		};

		struct ManagedDirectSystem : ManagedSystem
		{
			unique_ptr<DirectSystem> system{};
		};

		struct ManagedIndirectSystem : ManagedSystem
		{
			unique_ptr<IndirectSystem> system{};
			// contains messages that the system needs to process before it starts its update
			struct MessageQueue
			{
				vector<MessageStreamEntityAdded> entityAddedStreams{};
				vector<MessageStreamComponentChanged> componentChangedStreams{};
				vector<MessageStreamEntityRemoved> entityRemovedStreams{};

				void clear();
				bool empty() const;
			} messageQueue{};
		};

		struct Pipeline
		{
			// every time the schedule sends a system to be executed by a worker, it increments this value
			// after the system is done executing, the worker decrements it, and the scheduler must wait for
			// this value to become 0 before it can start a new frame
			ui32 systemsAtExecution = 0;
			ui32 executionFrame = 0;
			vector<ManagedDirectSystem> directSystems{};
			vector<ManagedIndirectSystem> indirectSystems{};
			optional<ui32> stepMicroSeconds{};
			vector<StableTypeId> writeComponents{};
		};

		// used by all pipelines to perform execution
		std::thread _schedulerThread{};

		// used for matching EntityID to physical entity and its components
		// this is needed when processing entity/component update messages
		std::unordered_map<EntityID, EntityLocation> _entitiesLocations{};

		// stores all antities and their components grouped by archetype
		std::unordered_map<ArchetypeFull, ArchetypeGroup> _archetypeGroupsFull{};
		// similar archetypes, like containing entities with multiple components of the same type,
		// will be considered as same archetype, so if you don't care about the components count,
		// but only about their presence, use this
		std::unordered_map<Archetype, vector<std::reference_wrapper<ArchetypeGroup>>> _archetypeGroups{};

		// stores Systems within their pipelines, used by the manager to iterate through the systems
		vector<Pipeline> _pipelines{};

		ui32 _lastComponentId = {0};

		ArchetypeReflector _archetypeReflector{};

		std::atomic<bool> _isStoppingExecution{false};

		std::atomic<bool> _isPausedExecution{false};
		std::mutex _executionPauseMutex{};
		std::condition_variable _executionPauseNotifier{};

		std::atomic<bool> _isSchedulerPaused{false};
		std::mutex _schedulerPausedMutex{};
		std::condition_variable _schedulerPausedNotifier{};

		EntityIDGenerator _idGenerator{};

	private:
		[[nodiscard]] ArchetypeGroup &FindArchetypeGroup(const ArchetypeFull &archetype, Array<const SerializedComponent> components);
		ArchetypeGroup &AddNewArchetypeGroup(const ArchetypeFull &archetype, Array<const SerializedComponent> components);
		void AddEntityToArchetypeGroup(const ArchetypeFull &archetype, ArchetypeGroup &group, EntityID entityId, Array<const SerializedComponent> components, MessageBuilder *messageBuilder);
		void StartScheduler(vector<unique_ptr<EntitiesStream>> &&streams);
		void SchedulerLoop();
		void ExecutePipeline(Pipeline &pipeline);
		void CalculateGroupsToExectute(const System *system, vector<std::reference_wrapper<ArchetypeGroup>> &groups);
		static void ProcessMessages(IndirectSystem &system, const ManagedIndirectSystem::MessageQueue &messageQueue);
	};
}