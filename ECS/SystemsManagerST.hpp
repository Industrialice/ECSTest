#pragma once

#include "SystemsManager.hpp"
#include "MessageBuilder.hpp"
#include "ArchetypeReflector.hpp"

namespace ECSTest
{
	class SystemsManagerST : public SystemsManager, public std::enable_shared_from_this<SystemsManagerST>
	{
		friend class ECSEntitiesST;

	protected:
		~SystemsManagerST() = default;
		SystemsManagerST(const shared_ptr<LoggerType> &logger);

	public:
		static shared_ptr<SystemsManagerST> New(const shared_ptr<LoggerType> &logger);

		[[nodiscard]] virtual Pipeline CreatePipeline(optional<TimeDifference> executionStep, bool isMergeIfSuchPipelineExists) override;
        [[nodiscard]] virtual PipelineInfo GetPipelineInfo(Pipeline pipeline) const override;
        [[nodiscard]] virtual ManagerInfo GetManagerInfo() const override;
        virtual void SetLogger(const shared_ptr<LoggerType> &logger) override;
        virtual void Register(unique_ptr<System> system, Pipeline pipeline) override;
		virtual void Unregister(TypeId systemType) override;
		virtual void Start(AssetsManager &&assetsManager, EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, vector<unique_ptr<IEntitiesStream>> &&streams) override;
		virtual void Pause(bool isWaitForStop) override; // you can call it multiple times, for example first time as Pause(false), and then as Pause(true) to wait for paused
		virtual void Resume() override;
		virtual void Stop(bool isWaitForStop) override;
		[[nodiscard]] virtual bool IsRunning() const override;
		[[nodiscard]] virtual bool IsPaused() const override;
		//virtual void StreamIn(vector<unique_ptr<IEntitiesStream>> &&streams) override;
		[[nodiscard]] virtual shared_ptr<IEntitiesStream> StreamOut() const override; // the manager must be paused
		
	private:
		struct ArchetypeGroup
		{
			struct ComponentArray
			{
				TypeId type{}; // of each component
				ui16 stride{}; // Components of that type per entity, 1 if there's only one. Any access to a particular component must be performed using index * stride
				ui16 sizeOf{}; // of each component
				ui16 alignmentOf{}; // of each component
				unique_ptr<byte[], AlignedMallocDeleter> data{}; // each component can be safely casted into class Component
				unique_ptr<ComponentID[], MallocDeleter> ids{}; // ComponentID, used only for components that allow multiple components of that type to be attached to an entity
				bool isUnique{}; // indicates whether other components of the same type can be attached to an entity
			};

			unique_ptr<ComponentArray[]> components{}; // essentially a 2D array where rows count = uniqueTypedComponentsCount, columns count is computed per row as entitiesCount * stride
			unique_ptr<EntityID[]> entities{};
            unique_ptr<TypeId[]> tags{}; // tag components of this archetype group
            ui16 tagsCount{};
			ui16 uniqueTypedComponentsCount{};
			ui32 entitiesReservedCount{};
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
			ui32 executedAt{}; // last executed frame, gets set to PipelineData::executionFrame at first execution attempt on a new frame
            ui32 executedTimes{};
            ControlsQueue controlsReceivedQueue{}, controlsToSendQueue{}; // 2 separate queues are necessary because you can send new control actions from ControlInput method
		};

		struct ManagedDirectSystem : ManagedSystem
		{
			unique_ptr<BaseDirectSystem> system{};
		};

		struct ManagedIndirectSystem : ManagedSystem
		{
			unique_ptr<BaseIndirectSystem> system{};
			// contains messages that the system needs to process before it starts its update
			struct MessageQueue
			{
				vector<MessageStreamEntityAdded> entityAddedStreams{};
                vector<MessageStreamComponentAdded> componentAddedStreams{};
				vector<MessageStreamComponentChanged> componentChangedStreams{};
                vector<MessageStreamComponentRemoved> componentRemovedStreams{};
				vector<MessageStreamEntityRemoved> entityRemovedStreams{};

				void clear();
				bool empty() const;
			} messageQueue{};
		};

		struct PipelineData
		{
			// every time the schedule sends a system to be executed by a worker, it increments this value
			// after the system is done executing, the worker decrements it, and the scheduler must wait for
			// this value to become 0 before it can start a new frame
			MovableAtomic<ui32> executionFrame = 0;
			vector<ManagedDirectSystem> directSystems{};
			vector<ManagedIndirectSystem> indirectSystems{};
			optional<TimeDifference> executionStep{};
			MovableAtomic<TimeDifference> timeSpentExecuting{};
            TimeMoment lastExecutedTime{};
			vector<TypeId> writeComponents{}; // list of components requested for write by the systems of this pipeline
		};

		// used by all pipelines to perform execution
		std::thread _schedulerThread{};

		// used for matching EntityID to physical entity and its components
		// this is needed when processing entity/component update messages
		// EntityID's hint will be an index in this array
		vector<EntityLocation> _entitiesLocations{};

		// stores all antities and their components grouped by archetype
		std::unordered_map<ArchetypeFull, ArchetypeGroup> _archetypeGroupsFull{};
		// similar archetypes, like containing entities with multiple components of the same type,
		// will be considered as same archetype, so if you don't care about the components count,
		// but only about their presence, use this
		std::unordered_map<Archetype, vector<std::reference_wrapper<ArchetypeGroup>>> _archetypeGroups{};

		// stores Systems within their pipelines, used by the manager to iterate through the systems
		vector<PipelineData> _pipelines{};

		ArchetypeReflector _archetypeReflector{};

		std::atomic<bool> _isStoppingExecution{false};

		std::atomic<bool> _isPausedExecution{false};
		std::mutex _executionPauseMutex{};
		std::condition_variable _executionPauseNotifier{};

		std::atomic<bool> _isSchedulerPaused{false};
		std::mutex _schedulerPausedMutex{};
		std::condition_variable _schedulerPausedNotifier{};

		EntityIDGenerator _entityIdGenerator{};
        ComponentIDGenerator _componentIdGenerator{};
		UniqueIdManager _entityHintGenerator{};

        std::atomic<TimeDifference> _timeSinceStartAtomic{};
        TimeDifference _timeSinceStart{};
        TimeMoment _currentTime{};

        shared_ptr<LoggerType> _logger = make_shared<LoggerType>();

        vector<SerializedComponent> _tempComponents{};
        vector<Array<byte>> _tempArrayArgs{};
		vector<NonUnique<byte>> _tempNonUniqueArgs{};
        vector<void *> _tempArgs{};

        MessageBuilder _tempMessageBuilder{};

		AssetsManager _assetsManager{};

        static constexpr string_view selfName = "ECSSingleThreaded";

	private:
		[[nodiscard]] ArchetypeGroup &FindArchetypeGroup(const ArchetypeFull &archetype, Array<const SerializedComponent> components);
		ArchetypeGroup &AddNewArchetypeGroup(const ArchetypeFull &archetype, Array<const SerializedComponent> components);
		void AddEntityToArchetypeGroup(const ArchetypeFull &archetype, ArchetypeGroup &group, EntityID entityId, Array<const SerializedComponent> components, MessageBuilder *messageBuilder);
		void StartScheduler(vector<unique_ptr<IEntitiesStream>> &streams);
		void SchedulerLoop();
		void ExecutePipeline(PipelineData &pipeline, TimeDifference timeSinceLastFrame);
		static void ProcessMessagesAndClear(BaseIndirectSystem &system, ManagedIndirectSystem::MessageQueue &messageQueue, System::Environment &env);
        void ExecuteIndirectSystem(BaseIndirectSystem &system, ManagedIndirectSystem::MessageQueue &messageQueue, ControlsQueue &controlsReceivedQueue, ControlsQueue &controlsToSendQueue, System::Environment &env);
        void ExecuteDirectSystem(BaseDirectSystem &system, ControlsQueue &controlsReceivedQueue, ControlsQueue &controlsToSendQueue, System::Environment &env);
        static void ProcessControlsQueueAndClear(System &system, ControlsQueue &controlsQueue);
        void PassControlsToOtherSystemsAndClear(ControlsQueue &controlsQueue, System *systemToIgnore);
        void PatchComponentAddedMessages(MessageBuilder &messageBuilder);
        void PatchEntityRemovedArchetypes(MessageBuilder &messageBuilder);
        void UpdateECSFromMessagesAndCreateArchetypedMessageBuilders(MessageBuilder &messageBuilder);
        void PassMessagesToIndirectSystemsAndClear(MessageBuilder &messageBuilder, System *systemToIgnore);
        static bool SendControlActionToQueue(ControlsQueue &controlsQueue, const ControlAction &action);
	};
}