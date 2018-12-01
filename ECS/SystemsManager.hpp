#pragma once

#include "System.hpp"
#include "Entity.hpp"
#include "World.hpp"
#include "DIWRSpinLock.hpp"
#include <ListenerHandle.hpp>

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
		//using ListenerHandle = ListenerLocation::ListenerHandle;
		//using OnSystemExecutedCallbackType = function<void(const System &)>;

		SystemsManager() = default;
		//[[nodiscard]] ListenerHandle OnSystemExecuted(TypeId systemType, OnSystemExecutedCallbackType callback);
		//void RemoveListener(ListenerHandle &listener);
		//void Register(System &system, optional<ui32> stepMicroSeconds, const vector<TypeId> &runBefore, const vector<TypeId> &runAfter, std::thread::id affinityThread);
        void Register(unique_ptr<System> system, optional<ui32> stepMicroSeconds);
		void Unregister(TypeId systemType);
		void Spin(World &&world, vector<std::thread> &&threads);

	private:
        struct ArchetypeGroup
        {
            struct ComponentArray
            {
                TypeId type{};
                ui16 stride{}; // Components of that type per entity, 1 if there's only one. Components must be indexed using this stride
                ui16 sizeOf{};
                ui16 alignmentOf{};
                ui16 reservedCount{};
                unique_ptr<ui8[], AlignedMallocDeleter> data{};
                DIWRSpinLock lock{};
            };

            unique_ptr<EntityID[]> entities{};
            vector<ComponentArray> components{};
            DIWRSpinLock lock{};

            uiw EntriesCount() const
            {
                return components.size();
            }
        };

        struct EntityLocation
        {
            EntityArchetype archetype{};
            ui32 index{};
        };

		//struct OnSystemExecutedData
		//{
		//	OnSystemExecutedCallbackType _callback;
		//	ui64 _id;
		//};

		//struct PendingOnSystemExecutedData
		//{
		//	OnSystemExecutedData _data;
		//	TypeId _type;
		//};

		struct WorkerThread
		{
			std::thread _thread{};
			std::atomic<bool> _isWaitingForWork{true};
			std::atomic<bool> _isExiting{false};
		};

        // used for matching EntityID to physical entity and its components
        // this is needed when processing entity/component update messages
        // owned by the scheduler threads
        std::unordered_map<EntityID, EntityLocation> _entitiesLocations{};

        // stores all antities and their components grouped by archetype
        // synchronized between all the threads
        std::unordered_map<EntityArchetype, ArchetypeGroup> _archetypeGroups{};

        // stores Systems within their pipelines, used by the manager to iterate through
        // the systems
        // owned by the scheduler threads
        std::unordered_map<optional<ui32>, vector<unique_ptr<System>>> _pipelines{};

		vector<WorkerThread> _threads{};
		std::condition_variable _workerDoneNotifier{};
		std::mutex _workerReportMutex{};
		ui32 _workersVacantCount = 0;
		//vector<PendingOnSystemExecutedData> _pendingOnSystemExecutedDatas{};
		//ui64 _onSystemExecutedCurrentId = 0;
		//shared_ptr<ListenerLocation> _listenerLocation = make_shared<ListenerLocation>(*this);
	};
}