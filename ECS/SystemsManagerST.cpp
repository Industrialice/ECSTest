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
    return PipelineGroup();
}

void SystemsManagerST::Register(unique_ptr<System> system, PipelineGroup pipelineGroup)
{}

void SystemsManagerST::Unregister(StableTypeId systemType)
{}

void SystemsManagerST::Start(EntityIDGenerator && idGenerator, vector<WorkerThread>&& workers, vector<unique_ptr<EntitiesStream>>&& streams)
{}

void SystemsManagerST::Pause(bool isWaitForStop)
{}

void SystemsManagerST::Resume()
{}

void SystemsManagerST::Stop(bool isWaitForStop)
{}

bool SystemsManagerST::IsRunning() const
{
    return false;
}

bool SystemsManagerST::IsPaused() const
{
    return false;
}

shared_ptr<EntitiesStream> SystemsManagerST::StreamOut() const
{
    return shared_ptr<EntitiesStream>();
}
