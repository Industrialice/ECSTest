#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
    constexpr bool IsMTECS = false;
    constexpr ui32 EntitiesToTest = 1000;
}

COMPONENT(CosineResultComponent)
{
    f32 value;
};
static_assert(sizeof(CosineResultComponent) == sizeof(f32), "Unexpected sizeof");

COMPONENT(SinusResultComponent)
{
    f32 value;
};
static_assert(sizeof(SinusResultComponent) == sizeof(f32), "Unexpected sizeof");

COMPONENT(SourceComponent)
{
    f32 value;
};
static_assert(sizeof(SourceComponent) == sizeof(f32), "Unexpected sizeof");

TAG_COMPONENT(Group0Tag);
TAG_COMPONENT(Group1Tag);
TAG_COMPONENT(Group2Tag);
TAG_COMPONENT(Group3Tag);

DIRECT_SYSTEM(System0)
{
    DIRECT_ACCEPT_COMPONENTS(Array<CosineResultComponent> &cosine, Array<SinusResultComponent> &sinus, const Array<SourceComponent> &sources, RequiredComponent<Group0Tag>)
    {
        ASSUME(cosine.size() == sinus.size() && sinus.size() == sources.size());
        for (uiw index = 0; index < cosine.size(); ++index)
        {
            cosine[index].value = cos(sources[index].value);
            sinus[index].value = sin(sources[index].value);
        }
    }
};

DIRECT_SYSTEM(System1)
{
    DIRECT_ACCEPT_COMPONENTS(Array<CosineResultComponent> &cosine, Array<SinusResultComponent> &sinus, const Array<SourceComponent> &sources, RequiredComponent<Group1Tag>)
    {
        ASSUME(cosine.size() == sinus.size() && sinus.size() == sources.size());
        for (uiw index = 0; index < cosine.size(); ++index)
        {
            cosine[index].value = cos(sources[index].value);
            sinus[index].value = sin(sources[index].value);
        }
    }
};

DIRECT_SYSTEM(System2)
{
    DIRECT_ACCEPT_COMPONENTS(Array<CosineResultComponent> &cosine, Array<SinusResultComponent> &sinus, const Array<SourceComponent> &sources, RequiredComponent<Group2Tag>)
    {
        ASSUME(cosine.size() == sinus.size() && sinus.size() == sources.size());
        for (uiw index = 0; index < cosine.size(); ++index)
        {
            cosine[index].value = cos(sources[index].value);
            sinus[index].value = sin(sources[index].value);
        }
    }
};

DIRECT_SYSTEM(System3)
{
    DIRECT_ACCEPT_COMPONENTS(Array<CosineResultComponent> &cosine, Array<SinusResultComponent> &sinus, const Array<SourceComponent> &sources, RequiredComponent<Group3Tag>)
    {
        ASSUME(cosine.size() == sinus.size() && sinus.size() == sources.size());
        for (uiw index = 0; index < cosine.size(); ++index)
        {
            cosine[index].value = cos(sources[index].value);
            sinus[index].value = sin(sources[index].value);
        }
    }
};

static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, TestEntities &stream)
{
    auto generate = [&stream, &entityIdGenerator](auto group)
    {
        for (uiw index = 0; index < EntitiesToTest; ++index)
        {
            TestEntities::PreStreamedEntity entity;

            SourceComponent source;
            source.value = rand() % 4 - 2;

            StreamComponent(source, entity);
            StreamComponent(CosineResultComponent{}, entity);
            StreamComponent(SinusResultComponent{}, entity);
            StreamComponent(group, entity);

            stream.AddEntity(entityIdGenerator.Generate(), move(entity));
        }
    };

    generate(Group0Tag{});
    generate(Group1Tag{});
    generate(Group2Tag{});
    generate(Group3Tag{});
}

static void MesasureReference()
{
    auto entitiesToTest = EntitiesToTest;
    auto cosine = make_unique<CosineResultComponent[]>(entitiesToTest);
    auto sinus = make_unique<SinusResultComponent[]>(entitiesToTest);
    auto sources = make_unique<SourceComponent[]>(entitiesToTest);
    for (uiw index = 0; index < entitiesToTest; ++index)
    {
        sources[index].value = rand() % 4 - 2;
    }

    ui32 executedTimes = 0;
    TimeMoment start = TimeMoment::Now();
    TimeDifference diff{0_ms};
    for (;;)
    {
        for (uiw index = 0; index < entitiesToTest; ++index)
        {
            cosine[index].value = cos(sources[index].value);
            sinus[index].value = sin(sources[index].value);
        }

        ++executedTimes;
        diff = TimeMoment::Now() - start;
        if (diff > 1_s)
        {
            break;
        }
    }

    auto computed = executedTimes * entitiesToTest;
    auto time = diff.ToSec_f64();
    printf("%.2lfkk sin/cos per second (reference 1 thread)\n", (computed / time) / 1000 / 1000);
}

int main()
{
    StdLib::Initialization::Initialize({});

    auto idGenerator = EntityIDGenerator{};
    auto manager = SystemsManager::New(IsMTECS);
    auto stream = make_unique<TestEntities>();
    
    GenerateScene(idGenerator, *manager, *stream);

    auto pipeline = manager->CreatePipeline(nullopt, false);

    manager->Register<System0>(pipeline);
    manager->Register<System1>(pipeline);
    manager->Register<System2>(pipeline);
    manager->Register<System3>(pipeline);

    vector<WorkerThread> workers;
    if (IsMTECS)
    {
        workers.resize(SystemInfo::LogicalCPUCores());
    }

    MesasureReference();

    manager->Start(move(idGenerator), move(workers), move(stream));

    for (;;)
    {
        auto info = manager->GetManagerInfo();
        if (info.timeSinceStart > 1_s)
        {
            break;
        }
        std::this_thread::yield();
    }

    manager->Stop(true);
    auto managerInfo = manager->GetManagerInfo();
    auto pipelineInfo = manager->GetPipelineInfo(pipeline);

    auto computed = pipelineInfo.executedTimes * 4 * EntitiesToTest;
    auto time = managerInfo.timeSinceStart.ToSec_f64();
    printf("%.2lfkk sin/cos per second (ECS %s)\n", (computed / time) / 1000 / 1000, IsMTECS ? "multithreaded" : "singlethreaded");

    system("pause");
}