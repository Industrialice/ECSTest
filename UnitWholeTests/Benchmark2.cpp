#include "PreHeader.hpp"

using namespace ECSTest;

namespace
{
    constexpr bool IsMTECS = false;
	constexpr bool IsPhysicsFPSRestricted = true;
	constexpr bool IsPhysicsUsingComponentChangedHints = true;
    constexpr ui32 EntitiesToTest = 32768;
    constexpr ui32 PhysicsUpdatesPerFrame = 100;
    constexpr ui32 RendererDrawPerFrame = 100;

    COMPONENT(Physics)
    {
        f32 mass;
        f32 linearDamping;
        f32 angularDamping;
        Vector3 centerOfMass;
        Vector3 inertiaTensor;
        Quaternion inertiaTensorRotation;
        Vector3 linearVelocity;
        Vector3 angularVelocity;
        ui32 solverIterations;
        ui32 solverVelocityIterations;
        f32 maxAngularVelocity;
        f32 sleepThreshold;
        f32 wakeCounter;
        f32 contactOffset;
        f32 restOffset;
    };

    COMPONENT(MeshRenderer)
    {
        std::array<char, 260> mesh;
    };

    COMPONENT(Position)
    {
        Vector3 position;

        Position() = default;
        Position(const Vector3 &position) : position(position) {}
    };

    COMPONENT(Rotation)
    {
        Quaternion rotation;

        Rotation() = default;
        Rotation(const Quaternion &rotation) : rotation(rotation) {}
    };

    COMPONENT(MeshCollider)
    {
        bool isTrigger;
        std::array<char, 260> mesh;
    };

    INDIRECT_SYSTEM(PhysicsSystem)
    {
        INDIRECT_ACCEPT_COMPONENTS(Array<Position> &, Array<Rotation> &, const Array<MeshCollider> &, const Array<Physics> &);

        struct Data
        {
            Vector3 pos;
            Quaternion rot;
            Physics properties;
        };

        virtual void Update(Environment &env) override
        {
            if (_updateIdsList.empty())
            {
                ui32 count = 0;
                for (const auto &[id, data] : _entities)
                {
					_updateIdsList.push_back(id);
					_updatePositionsList.push_back(data.pos);
					_updateRotationsList.push_back(data.rot);

                    if (++count == PhysicsUpdatesPerFrame)
                    {
                        break;
                    }
                }
            }

			uiw updateCount = _updateIdsList.size();

			if (IsPhysicsUsingComponentChangedHints)
			{
				env.messageBuilder.ComponentChangedHint(Position::Description(), updateCount);
				env.messageBuilder.ComponentChangedHint(Rotation::Description(), updateCount);
			}

			for (uiw index = 0; index < updateCount; ++index)
			{
				env.messageBuilder.ComponentChanged(_updateIdsList[index], Position{_updatePositionsList[index]});
			}
			for (uiw index = 0; index < updateCount; ++index)
			{
				env.messageBuilder.ComponentChanged(_updateIdsList[index], Rotation{_updateRotationsList[index]});
			}
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entry : stream)
            {
                Data data;
                data.pos = entry.GetComponent<Position>().position;
                data.rot = entry.GetComponent<Rotation>().rotation;
                data.properties = entry.GetComponent<Physics>();
                _entities[entry.entityID] = data;
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            for (auto &entry : stream)
            {
                Data data;
                data.pos = entry.GetComponent<Position>().position;
                data.rot = entry.GetComponent<Rotation>().rotation;
                data.properties = entry.GetComponent<Physics>();
                _entities[entry.entityID] = data;
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
            if (stream.Type() == Position::GetTypeId())
            {
                for (auto &entry : stream)
                {
                    _entities[entry.entityID].pos = entry.Cast<Position>(stream.Type()).position;
                }
            }
            else if (stream.Type() == Rotation::GetTypeId())
            {
                for (auto &entry : stream)
                {
                    _entities[entry.entityID].rot = entry.Cast<Rotation>(stream.Type()).rotation;
                }
            }
            else if (stream.Type() == Physics::GetTypeId())
            {
                for (auto &entry : stream)
                {
                    _entities[entry.entityID].properties = entry.Cast<Physics>(stream.Type());
                }
            }
            else if (stream.Type() == MeshCollider::GetTypeId())
            {
            }
            else
            {
                HARDBREAK;
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            if (stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == Physics::GetTypeId() || stream.Type() == MeshCollider::GetTypeId())
            {
                for (auto &entry : stream)
                {
                    _entities.erase(entry.entityID);
                }
				_updatePositionsList.clear();
				_updateRotationsList.clear();
				_updateIdsList.clear();
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entry : stream)
            {
                _entities.erase(entry);
            }
			_updatePositionsList.clear();
			_updateRotationsList.clear();
			_updateIdsList.clear();
        }

    private:
        std::unordered_map<EntityID, Data> _entities{};
        std::vector<Vector3> _updatePositionsList{};
		std::vector<Quaternion> _updateRotationsList{};
		std::vector<EntityID> _updateIdsList{};
    };

    INDIRECT_SYSTEM(RendererSystem)
    {
        INDIRECT_ACCEPT_COMPONENTS(const Array<Position> &, const Array<Rotation> &, const Array<MeshRenderer> &);

        struct Data
        {
            EntityID id;
            Vector3 pos;
            Quaternion rot;
            MeshRenderer mesh;
        };

        virtual void Update(Environment &env) override
        {
			//std::this_thread::sleep_for(1ms);
            ui32 count = 0;
            for (auto it = _linear.begin(); count < RendererDrawPerFrame && it != _linear.end(); ++it, ++count)
            {
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entry : stream)
            {
                Data data;
                data.pos = entry.GetComponent<Position>().position;
                data.rot = entry.GetComponent<Rotation>().rotation;
                data.mesh = entry.GetComponent<MeshRenderer>();
				_entities[entry.entityID] = (ui32)_linear.size();
				_linear.emplace_back(data);
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            for (auto &entry : stream)
            {
                Data data;
                data.pos = entry.GetComponent<Position>().position;
                data.rot = entry.GetComponent<Rotation>().rotation;
                data.mesh = entry.GetComponent<MeshRenderer>();
				_entities[entry.entityID] = (ui32)_linear.size();
				_linear.emplace_back(data);
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
            if (stream.Type() == Position::GetTypeId())
            {
				for (auto &entry : stream)
				{
					_linear[_entities[entry.entityID]].pos = entry.Cast<Position>(stream.Type()).position;
				}
            }
            else if (stream.Type() == Rotation::GetTypeId())
            {
				for (auto &entry : stream)
				{
					_linear[_entities[entry.entityID]].rot = entry.Cast<Rotation>(stream.Type()).rotation;
				}
            }
            else if (stream.Type() == MeshRenderer::GetTypeId())
            {
                for (auto &entry : stream)
                {
					_linear[_entities[entry.entityID]].mesh = entry.Cast<MeshRenderer>(stream.Type());
                }
            }
            else
            {
                HARDBREAK;
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            if (stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == MeshRenderer::GetTypeId())
            {
                for (auto &entry : stream)
                {
					RemoveEntity(entry.entityID);
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entry : stream)
            {
				RemoveEntity(entry);
            }
        }

	private:
		void RemoveEntity(EntityID id)
		{
			auto it = _entities.find(id);
			ASSUME(it != _entities.end());
			EntityID idToPatch = _linear.back().id;
			_linear[it->second] = _linear.back();
			_linear.pop_back();
			_entities[idToPatch] = it->second;
			_entities.erase(it);
		}

    private:
        std::unordered_map<EntityID, ui32> _entities{};
        vector<Data> _linear{};
    };

    static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
    {
        for (uiw index = 0; index < EntitiesToTest; ++index)
        {
            EntitiesStream::EntityData entity;

            Position pos = {Vector3((f32)rand(), (f32)rand(), (f32)rand())};
            entity.AddComponent(pos);

            Rotation rot = Quaternion::FromEuler({(f32)rand(), (f32)rand(), (f32)rand()});
            entity.AddComponent(rot);

            Physics phy;
            entity.AddComponent(phy);

            MeshRenderer meshRenderer;
            entity.AddComponent(meshRenderer);

            MeshCollider collider;
            entity.AddComponent(collider);

            stream.AddEntity(entityIdGenerator.Generate(), move(entity));
        }
    }
}

void Benchmark2()
{
    StdLib::Initialization::Initialize({});

	printf("ECS multithreaded: %s\n", IsMTECS ? "yes" : "no");
	printf("IsPhysicsFPSRestricted: %s\n", IsPhysicsFPSRestricted ? "yes" : "no");
	printf("IsPhysicsUsingComponentChangedHints: %s\n", IsPhysicsUsingComponentChangedHints ? "yes" : "no");
	printf("EntitiesToTest: %u\n", EntitiesToTest);
	printf("PhysicsUpdatesPerFrame: %u\n", PhysicsUpdatesPerFrame);
	printf("RendererDrawPerFrame: %u\n", RendererDrawPerFrame);

    auto logger = make_shared<Logger<string_view, true>>();
    auto handle0 = logger->OnMessage(LogRecipient);

    auto idGenerator = EntityIDGenerator{};
    auto manager = SystemsManager::New(IsMTECS, logger);
    auto stream = make_unique<EntitiesStream>();

    auto before = TimeMoment::Now();
    GenerateScene(idGenerator, *manager, *stream);
    auto after = TimeMoment::Now();
    printf("Generating scene took %.2lfs\n", (after - before).ToSec());

    auto physicsPipeline = manager->CreatePipeline(IsPhysicsFPSRestricted ? optional(SecondsFP64(1.0 / 60.0)) : nullopt, false);
    auto rendererPipeline = manager->CreatePipeline(nullopt, false);

    manager->Register<PhysicsSystem>(physicsPipeline);
    manager->Register<RendererSystem>(rendererPipeline);

    vector<WorkerThread> workers;
    if (IsMTECS)
    {
        workers.resize(SystemInfo::LogicalCPUCores());
    }

    manager->Start(move(idGenerator), move(workers), move(stream));

    TimeDifference lastDifference;
    ui32 lastExecutedRenderer = 0, lastExecutedPhysics = 0;
	TimeDifference rendererLastSpent, physicsLastSpent;

    std::vector<f32> fpsHistory{};
	char backBuf[512];
	memset(backBuf, '\b', sizeof(backBuf));
	int lastPrinted = 510;

    for (;;)
    {
        auto managerInfo = manager->GetManagerInfo();
        auto timeDiff = managerInfo.timeSinceStart - lastDifference;
        if (timeDiff >= 1_s)
        {
            auto rendererInfo = manager->GetPipelineInfo(rendererPipeline);
            ui32 rexecDiff = rendererInfo.executedTimes - lastExecutedRenderer;
            
            auto physicsInfo = manager->GetPipelineInfo(physicsPipeline);
            ui32 pexecDiff = physicsInfo.executedTimes - lastExecutedPhysics;

			f32 diffRev = 1.0f / timeDiff.ToSec();

            f32 rfps = (f32)rexecDiff * diffRev;
            f32 pfps = (f32)pexecDiff * diffRev;

			f32 rendererSpent = (rendererInfo.timeSpentExecuting - rendererLastSpent).ToMSec() * diffRev;
			f32 physicsSpent = (physicsInfo.timeSpentExecuting - physicsLastSpent).ToMSec() * diffRev;

			backBuf[lastPrinted + 1] = '\0';
			printf(backBuf);
			int printed = printf("renderer %.1ffps (%.2lfms, %.2lfms), physics %.1ffps (%.2lfms, %.2lfms)  ", rfps, rendererSpent, rendererSpent / rfps, pfps, physicsSpent, physicsSpent / pfps);
			backBuf[lastPrinted + 1] = '\b';
			lastPrinted = printed;
            
            fpsHistory.push_back(rfps);
            lastExecutedRenderer = rendererInfo.executedTimes;
			rendererLastSpent = rendererInfo.timeSpentExecuting;
            lastExecutedPhysics = physicsInfo.executedTimes;
			physicsLastSpent = physicsInfo.timeSpentExecuting;
            lastDifference = managerInfo.timeSinceStart;
        }
		std::this_thread::sleep_for(1ms);
    }

    manager->Stop(true);

    f32 avg = std::accumulate(fpsHistory.begin(), fpsHistory.end(), 0.0f);
    printf("\nAverage FPS: %.2lf\n", avg / fpsHistory.size());
}