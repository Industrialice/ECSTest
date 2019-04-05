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
        [[nodiscard]] virtual shared_ptr<EntitiesStream> StreamOut() const override; // the system must be paused
    };
}