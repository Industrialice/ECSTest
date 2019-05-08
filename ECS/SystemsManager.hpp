#pragma once

#include "System.hpp"
#include "EntityID.hpp"
#include "IEntitiesStream.hpp"
#include "WokerThread.hpp"

namespace ECSTest
{
    class NOVTABLE SystemsManager
    {
        SystemsManager(SystemsManager &&) = delete;
        SystemsManager &operator = (SystemsManager &&) = delete;

    protected:
        ~SystemsManager() = default;
        SystemsManager() = default;

		using LoggerType = Logger<string_view, true>;

    public:
        static shared_ptr<SystemsManager> New(bool isMultiThreaded, const shared_ptr<LoggerType> &logger);

        class Pipeline
        {
            ui32 index{};
            friend class SystemsManagerMT;
            friend class SystemsManagerST;
        };

        struct PipelineInfo
        {
            ui32 executedTimes{};
            ui32 directSystems{};
            ui32 indirectSystems{};
            optional<TimeDifference> executionStep{};
			TimeDifference timeSpentExecuting{};
        };

        struct ManagerInfo
        {
            bool isMultiThreaded{};
            TimeDifference timeSinceStart{};
        };

        template <typename T> void Register(Pipeline pipeline)
        {
            return Register(std::make_unique<T>(), pipeline);
        }

        void Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers)
        {
            vector<unique_ptr<IEntitiesStream>> streams;
            return Start(move(idGenerator), move(workers), move(streams));
        }

        void Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, unique_ptr<IEntitiesStream> &&stream)
        {
            vector<unique_ptr<IEntitiesStream>> streams;
            streams.push_back(move(stream));
            return Start(move(idGenerator), move(workers), move(streams));
        }

        [[nodiscard]] virtual Pipeline CreatePipeline(optional<TimeDifference> executionStep, bool isMergeIfSuchPipelineExists) = 0;
        [[nodiscard]] virtual PipelineInfo GetPipelineInfo(Pipeline pipeline) const = 0;
        [[nodiscard]] virtual ManagerInfo GetManagerInfo() const = 0;
        virtual void SetLogger(const shared_ptr<LoggerType> &logger) = 0;
        virtual void Register(unique_ptr<System> system, Pipeline pipeline) = 0;
        virtual void Unregister(StableTypeId systemType) = 0;
        virtual void Start(EntityIDGenerator &&idGenerator, vector<WorkerThread> &&workers, vector<unique_ptr<IEntitiesStream>> &&streams) = 0;
        virtual void Pause(bool isWaitForStop) = 0; // you can call it multiple times, for example first time as Pause(false), and then as Pause(true) to wait for paused
        virtual void Resume() = 0;
        virtual void Stop(bool isWaitForStop) = 0;
        [[nodiscard]] virtual bool IsRunning() const = 0;
        [[nodiscard]] virtual bool IsPaused() const = 0;
        //virtual void StreamIn(vector<unique_ptr<IEntitiesStream>> &&streams) = 0;
        [[nodiscard]] virtual shared_ptr<IEntitiesStream> StreamOut() const = 0; // the manager must be paused
	};
}