#pragma once

#include "System.hpp"
#include "EntityID.hpp"
#include "Archetype.hpp"
#include "DIWRSpinLock.hpp"
#include "EntitiesStream.hpp"
#include "MessageBuilder.hpp"
#include "ArchetypeReflector.hpp"
#include "WokerThread.hpp"
//#include <ListenerHandle.hpp>

namespace ECSTest
{
    class SystemsManager
    {
        //struct ListenerLocation;
        //static void RemoveListener(ListenerLocation *instance, void *handle);

        //struct ListenerLocation
        //{
        //	SystemsManager *_manager;
        //	using ListenerHandle = TListenerHandle<ListenerLocation, RemoveListener, ui64>;
        //	ListenerLocation(SystemsManager &manager) : _manager(&manager) {}
        //};

        SystemsManager(SystemsManager &&) = delete;
        SystemsManager &operator = (SystemsManager &&) = delete;

    public:
        class PipelineGroup
        {
            ui32 index{};
            friend SystemsManager;
        };

        //using ListenerHandle = ListenerLocation::ListenerHandle;
        //using OnSystemExecutedCallbackType = function<void(const System &)>;

        SystemsManager() = default;
        //[[nodiscard]] ListenerHandle OnSystemExecuted(StableTypeId systemType, OnSystemExecutedCallbackType callback);
        //void RemoveListener(ListenerHandle &listener);
        //void Register(System &system, optional<ui32> stepMicroSeconds, const vector<StableTypeId> &runBefore, const vector<StableTypeId> &runAfter, std::thread::id affinityThread);
        PipelineGroup CreatePipelineGroup(optional<ui32> stepMicroSeconds, bool isMergeIfSuchPipelineExists);
        void Register(unique_ptr<System> system, PipelineGroup pipelineGroup);
        void Unregister(StableTypeId systemType);
        void Start(vector<WorkerThread> &&workers, Array<shared_ptr<EntitiesStream>> streams);
        void Stop(bool isWaitForStop);
        bool IsRunning();
        void StreamIn(Array<shared_ptr<EntitiesStream>> streams);

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
                unique_ptr<ui32[], MallocDeleter> ids{}; // ComponentID, used only for components that allow multiple components of that type to be attached to an entity
                // must lock it first before accessing `data` if you don't have exclusive
                // access to this group, other fields can be accessed without this lock
                DIWRSpinLock lock{};
                bool isUnique{}; // indicates whether other components of the same type can be attached to an entity
            };

            unique_ptr<ComponentArray[]> components{};
            ui16 uniqueTypedComponentsCount{};
            ui16 reservedCount{}; // final reserved count is computed as reservedCount * stride
            ui32 entitiesCount{};
            // must lock it first before accessing any other fields of the group
            // if you request exclusive write lock, locking components is not
            // required because you have exclusive access to all the content of this group
            DIWRSpinLock lock{};
        };

        struct EntityLocation
        {
            ArchetypeGroup *group{};
            ui32 index{};
        };

        //struct ComponentLocation
        //{
        //    ArchetypeGroup *group{};
        //    ui32 index{};
        //};

        //struct OnSystemExecutedData
        //{
        //	OnSystemExecutedCallbackType _callback;
        //	ui64 _id;
        //};

        //struct PendingOnSystemExecutedData
        //{
        //	OnSystemExecutedData _data;
        //	StableTypeId _type;
        //};

		struct ManagedSystem
		{
			ui32 executedAt{}; // last executed frame, gets set to Pipeline::executionFrame at first execution attempt on a new frame
			vector<std::reference_wrapper<ArchetypeGroup>> groupsToExecute{}; // group still left to be executed for the current frame
            vector<DIWRSpinLock::Unlocker> locks{}; // all currently held locks by the system
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
                vector<MessageStreamEntityAdded> entityAdded{};
                vector<MessageStreamEntityRemoved> entityRemoved{};
            } messageQueue{};
        };

        struct Pipeline
        {
            // every time the schedule sends a system to be executed by a worker, it increments this value
            // after the system is done executing, the worker decrements it, and the scheduler must wait for
            // this value to become 0 before it can start a new frame
            unique_ptr<std::atomic<ui32>> systemsAtExecution = make_unique<std::atomic<ui32>>(0);
			ui32 executionFrame = 0;
			vector<ManagedDirectSystem> directSystems{};
            vector<ManagedIndirectSystem> indirectSystems{};
            optional<ui32> stepMicroSeconds{};
        };

        // used by all pipelines to perform execution scheduling, all real
        // work including received message processing is done by workers
        std::thread _schedulerThread{};

        // used for matching EntityID to physical entity and its components
        // this is needed when processing entity/component update messages
        std::unordered_map<EntityID, EntityLocation> _entitiesLocations{};
        DIWRSpinLock _entitiesLocationsLock{};

        // stores all antities and their components grouped by archetype
        std::unordered_map<ArchetypeFull, ArchetypeGroup> _archetypeGroupsFull{};
        // similar archetypes, like containing entities with multiple components of the same type,
        // will be considered as same archetype, so if you don't care about the components count,
        // but only about their presence, use this
        std::unordered_map<Archetype, vector<std::reference_wrapper<ArchetypeGroup>>> _archetypeGroups{};
        // used to match component types and archetype groups
        //std::unordered_multimap<StableTypeId, ComponentLocation> _archetypeGroupsComponents{};
        // this lock is shared between all archetype groups arrays
        DIWRSpinLock _archetypeGroupsLock{};

        // stores Systems within their pipelines, used by the manager to iterate through the systems
        vector<Pipeline> _pipelines{};

        vector<WorkerThread> _workerThreads{};
        shared_ptr<pair<std::mutex, std::condition_variable>> _workerFinishedWorkNotifier = make_shared<pair<std::mutex, std::condition_variable>>();
        ui32 _workersVacantCount = 0;
        //vector<PendingOnSystemExecutedData> _pendingOnSystemExecutedDatas{};
        //ui64 _onSystemExecutedCurrentId = 0;
        //shared_ptr<ListenerLocation> _listenerLocation = make_shared<ListenerLocation>(*this);
		
        std::atomic<ui32> _lastComponentID = {0};

		ArchetypeReflector _archetypeReflector{};

        std::atomic<bool> _isStoppingExecution{false};

    private:
        void AssignComponentIDs(vector<ui32> &assignedIDs, Array<const EntitiesStream::ComponentDesc> components);
        [[nodiscard]] ArchetypeGroup &FindArchetypeGroup(ArchetypeFull archetype, const vector<ui32> &assignedIDs, Array<const EntitiesStream::ComponentDesc> components);
        void AddEntityToArchetypeGroup(ArchetypeFull archetype, ArchetypeGroup &group, const EntitiesStream::StreamedEntity &entity, const vector<ui32> &assignedIDs, MessageBuilder &messageBuilder);
        [[nodiscard]] bool IsSystemAcceptsArchetype(Archetype archetype, Array<const System::RequestedComponent> systemComponents) const;
        void StartScheduler(Array<shared_ptr<EntitiesStream>> streams);
        void SchedulerLoop();
        void ExecutePipeline(Pipeline &pipeline);
        void CalculateGroupsToExectute(const System *system, vector<std::reference_wrapper<ArchetypeGroup>> &groups);
        WorkerThread &FindBestWorker();

        static void TaskProcessMessages(IndirectSystem &system, ManagedIndirectSystem::MessageQueue &messageQueue);
        static void TaskExecuteIndirectSystem(IndirectSystem &system, ManagedIndirectSystem::MessageQueue messageQueue, System::Environment env, std::atomic<ui32> &decrementAtCompletion, Array<DIWRSpinLock::Unlocker> locks);
	};
}