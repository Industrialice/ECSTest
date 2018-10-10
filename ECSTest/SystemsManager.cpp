#include "PreHeader.hpp"
#include "SystemsManager.hpp"

using namespace ECSTest;

void SystemsManager::RemoveListener(ListenerLocation *instance, void *handle)
{
	instance->_manager->RemoveListener(*(ListenerHandle *)handle);
}

auto SystemsManager::OnSystemExecuted(TypeId systemType, OnSystemExecutedCallbackType callback) -> ListenerHandle
{
	auto id = _onSystemExecutedCurrentId;
	++_onSystemExecutedCurrentId;
	ASSUME(_onSystemExecutedCurrentId < 1'000'000); // it's won't break anything by itself, but indicates that you have a bug that causes callback attachment spam
	
	for (auto &[step, group] : _systemGroups)
	{
		for (auto &system : group._systems)
		{
			if (system._system->Type() == systemType)
			{
				system._onExecutedCallbacks.push_back({callback, id});
				return {_listenerLocation, id};
			}
		}
	}
	
	_pendingOnSystemExecutedDatas.push_back({{callback, id}, systemType});
	return {_listenerLocation, id};
}

void SystemsManager::RemoveListener(ListenerHandle &listener)
{
	for (auto &[step, group] : _systemGroups)
	{
		for (auto &system : group._systems)
		{
			auto pred = [&listener](const OnSystemExecutedData &data)
			{
				return listener.Id() == data._id;
			};
			auto it = std::find_if(system._onExecutedCallbacks.begin(), system._onExecutedCallbacks.end(), pred);
			if (it != system._onExecutedCallbacks.end())
			{
				system._onExecutedCallbacks.erase(it);
				return;
			}
		}
	}

	auto pred = [&listener](const PendingOnSystemExecutedData &data)
	{
		return listener.Id() == data._data._id;
	};
	auto it = std::find_if(_pendingOnSystemExecutedDatas.begin(), _pendingOnSystemExecutedDatas.end(), pred);
	if (it != _pendingOnSystemExecutedDatas.end())
	{
		_pendingOnSystemExecutedDatas.erase(it);
		return;
	}

	HARDBREAK;
}

void SystemsManager::Register(System &system, optional<ui32> stepMicroSeconds, const vector<TypeId> &runBefore, const vector<TypeId> &runAfter, std::thread::id affinityThread)
{
}

void SystemsManager::Unregister(TypeId systemType)
{
}

void SystemsManager::Spin(class World &world, vector<std::thread> &&threads)
{
}
