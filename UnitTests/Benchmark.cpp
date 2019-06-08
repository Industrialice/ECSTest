#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
	static constexpr bool IsMTECS = false;
	static constexpr ui32 EntitiesToTest = 1000;
}

volatile ui32 EntitiesToTestExternal = EntitiesToTest;
volatile f32 ExternalF32;

class BenchmarkClass
{
public:
	BenchmarkClass()
	{
		auto idGenerator = EntityIDGenerator{};
		auto manager = SystemsManager::New(IsMTECS, Log);
		auto stream = make_unique<EntitiesStream>();

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
			if (info.timeSinceStart >= 1_s)
			{
				break;
			}
			std::this_thread::sleep_for(1ms);
		}

		manager->Stop(true);
		auto managerInfo = manager->GetManagerInfo();
		auto pipelineInfo = manager->GetPipelineInfo(pipeline);

		auto computed = pipelineInfo.executedTimes * 4 * EntitiesToTest;
		auto time = managerInfo.timeSinceStart.ToSec_f64();
		Log->Info("", "%.2lfkk sin/cos per second (ECS %s)\n", (computed / time) / 1000 / 1000, IsMTECS ? "multithreaded" : "singlethreaded");
	}

    struct CosineResultComponent : Component<CosineResultComponent>
    {
        f32 value;
    };

    struct SinusResultComponent : Component<SinusResultComponent>
    {
        f32 value;
    };

    struct SourceComponent : Component<SourceComponent>
    {
        f32 value;
    };

	struct Group0Tag : TagComponent<Group0Tag> {};
	struct Group1Tag : TagComponent<Group1Tag> {};
	struct Group2Tag : TagComponent<Group2Tag> {};
	struct Group3Tag : TagComponent<Group3Tag> {};

    struct System0 : DirectSystem<System0>
    {
        void Accept(Array<CosineResultComponent> &cosine, Array<SinusResultComponent> &sinus, const Array<SourceComponent> &sources, RequiredComponent<Group0Tag>)
        {
            ASSUME(cosine.size() == sinus.size() && sinus.size() == sources.size());
            for (uiw index = 0; index < cosine.size(); ++index)
            {
                cosine[index].value = cos(sources[index].value);
                sinus[index].value = sin(sources[index].value);
            }
        }
    };

    struct System1 : DirectSystem<System1>
    {
		void Accept(Array<CosineResultComponent> &cosine, Array<SinusResultComponent> &sinus, const Array<SourceComponent> &sources, RequiredComponent<Group1Tag>)
        {
            ASSUME(cosine.size() == sinus.size() && sinus.size() == sources.size());
            for (uiw index = 0; index < cosine.size(); ++index)
            {
                cosine[index].value = cos(sources[index].value);
                sinus[index].value = sin(sources[index].value);
            }
        }
    };

    struct System2 : DirectSystem<System2>
    {
		void Accept(Array<CosineResultComponent> &cosine, Array<SinusResultComponent> &sinus, const Array<SourceComponent> &sources, RequiredComponent<Group2Tag>)
        {
            ASSUME(cosine.size() == sinus.size() && sinus.size() == sources.size());
            for (uiw index = 0; index < cosine.size(); ++index)
            {
                cosine[index].value = cos(sources[index].value);
                sinus[index].value = sin(sources[index].value);
            }
        }
    };

    struct System3 : DirectSystem<System3>
    {
		void Accept(Array<CosineResultComponent> &cosine, Array<SinusResultComponent> &sinus, const Array<SourceComponent> &sources, RequiredComponent<Group3Tag>)
        {
            ASSUME(cosine.size() == sinus.size() && sinus.size() == sources.size());
            for (uiw index = 0; index < cosine.size(); ++index)
            {
                cosine[index].value = cos(sources[index].value);
                sinus[index].value = sin(sources[index].value);
            }
        }
    };

    static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
    {
		stream.HintTotal(EntitiesToTest * 4);

        auto generate = [&stream, &entityIdGenerator](auto group)
        {
            for (uiw index = 0; index < EntitiesToTest; ++index)
            {
                EntitiesStream::EntityData entity;

                SourceComponent source;
                source.value = static_cast<f32>(rand() % 4 - 2);

                entity.AddComponent(source);
                entity.AddComponent(CosineResultComponent{});
                entity.AddComponent(SinusResultComponent{});
                entity.AddComponent(group);

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
        auto entitiesToTest = EntitiesToTestExternal;
        auto cosine = make_unique<CosineResultComponent[]>(entitiesToTest);
        auto sinus = make_unique<SinusResultComponent[]>(entitiesToTest);
        auto sources = make_unique<SourceComponent[]>(entitiesToTest);
        for (uiw index = 0; index < entitiesToTest; ++index)
        {
            sources[index].value = static_cast<f32>(rand() % 4 - 2);
        }

		f32 reciprocal = 1.0f / RAND_MAX;
		f32 halfReciprocal = reciprocal * 0.5f;
        ui32 executedTimes = 0;
        TimeMoment start = TimeMoment::Now();
        TimeDifference diff{};
        for (;;)
        {
            for (uiw index = 0; index < entitiesToTest; ++index)
            {
                cosine[index].value = cos(sources[index].value);
                sinus[index].value = sin(sources[index].value);
            }

            ++executedTimes;
            diff = TimeMoment::Now() - start;
            if (diff >= 1_s)
            {
                break;
            }
        }

		uiw index = static_cast<uiw>(rand()) % entitiesToTest;
		ExternalF32 = cosine[index].value;
		ExternalF32 = sinus[index].value;

        auto computed = executedTimes * entitiesToTest;
        auto time = diff.ToSec_f64();
        Log->Info("", "%.2lfkk sin/cos per second (reference 1 thread)\n", (computed / time) / 1000 / 1000);
    }
};

void Benchmark()
{
    StdLib::Initialization::Initialize({});
	BenchmarkClass test;
}