#pragma once

#include "System.hpp"
#include "EntityID.hpp"
#include "Archetype.hpp"
#include <DIWRSpinLock.hpp>
#include "EntitiesStream.hpp"
#include "MessageBuilder.hpp"
#include "ArchetypeReflector.hpp"
#include "WokerThread.hpp"
//#include <ListenerHandle.hpp>

namespace ECSTest
{
    class SystemsManager
    {
        SystemsManager(SystemsManager &&) = delete;
        SystemsManager &operator = (SystemsManager &&) = delete;

    protected:
        ~SystemsManager() = default;
        SystemsManager() = default;

    public:
        static shared_ptr<SystemsManager> New(bool isMultiThreaded);

        class PipelineGroup
        {
            ui32 index{};
            friend class SystemsManagerMT;
            friend class SystemsManagerST;
        };

        [[nodiscard]] virtual PipelineGroup CreatePipelineGroup(optional<TimeDifference> executionStep, bool isMergeIfSuchPipelineExists) = 0;
        virtual void Register(unique_ptr<System> system, PipelineGroup pipelineGroup) = 0;
        virtual void Unregister(StableTypeId systemType) = 0;
        virtual void Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, vector<unique_ptr<EntitiesStream>> &&streams) = 0;
        virtual void Pause(bool isWaitForStop) = 0; // you can call it multiple times, for example first time as Pause(false), and then as Pause(true) to wait for paused
        virtual void Resume() = 0;
        virtual void Stop(bool isWaitForStop) = 0;
        [[nodiscard]] virtual bool IsRunning() const = 0;
        [[nodiscard]] virtual bool IsPaused() const = 0;
        //virtual void StreamIn(vector<unique_ptr<EntitiesStream>> &&streams) = 0;
        [[nodiscard]] virtual shared_ptr<EntitiesStream> StreamOut() const = 0; // the manager must be paused
	};
}