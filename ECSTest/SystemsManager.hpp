#pragma once

#include "System.hpp"
#include <ListenerHandle.hpp>

namespace ECSTest
{
	class SystemsManager
	{
		struct ListenerLocation;
		static void RemoveListener(ListenerLocation *instance, void *handle);

		struct ListenerLocation
		{
			SystemsManager *_manager;

			using ListenerHandle = TListenerHandle<ListenerLocation, RemoveListener, ui64>;

			ListenerLocation(SystemsManager &manager) : _manager(&manager) {}
		};

		SystemsManager(SystemsManager &&) = delete;
		SystemsManager &operator = (SystemsManager &&) = delete;

	public:
		using ListenerHandle = ListenerLocation::ListenerHandle;
		using OnSystemExecutedCallbackType = function<void(const System &)>;

		SystemsManager() = default;
		ListenerHandle OnSystemExecuted(TypeId systemType, OnSystemExecutedCallbackType callback);
		void RemoveListener(ListenerHandle &listener);
		void Register(System &system, optional<ui32> stepMicroSeconds, const vector<TypeId> &runBefore, const vector<TypeId> &runAfter, std::thread::id affinityThread);
		void Unregister(TypeId systemType);
		void Spin(vector<std::thread> &&threads);

	private:
		struct OnSystemExecutedData
		{
			OnSystemExecutedCallbackType _callback;
			ui64 _id;
		};

		struct PendingOnSystemExecutedData
		{
			OnSystemExecutedData _data;
			TypeId _type;
		};

		struct SystemTask
		{
			System *_system;
			std::thread::id _affinityThread;
			vector<OnSystemExecutedData> _onExecutedCallbacks;
		};

		struct SystemGroup
		{
			vector<SystemTask> _systems;
		};

		struct WorkerThread
		{
			std::thread _thread{};
			std::atomic<bool> _isWaitingForWork{true};
			std::atomic<bool> _isExiting{false};
		};

		std::unordered_map<optional<ui32>, SystemGroup> _systemGroups{};
		vector<WorkerThread> _threads{};
		std::condition_variable _workerDoneNotifier{};
		std::mutex _workerReportMutex{};
		ui32 _workersVacantCount = 0;
		vector<PendingOnSystemExecutedData> _pendingOnSystemExecutedDatas{};
		ui64 _onSystemExecutedCurrentId = 0;
		shared_ptr<ListenerLocation> _listenerLocation = make_shared<ListenerLocation>(*this);
	};
}