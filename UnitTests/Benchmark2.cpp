#include "PreHeader.hpp"

using namespace ECSTest;

class Benchmark2Class
{
    static constexpr bool IsMTECS = false;
	static constexpr bool IsPhysicsFPSRestricted = false;
	static constexpr bool IsPhysicsUsingComponentChangedHints = true;
	static constexpr bool IsShuffleUpdatesOrder = false;
	static constexpr ui32 EntitiesToTest = 32768;
	static constexpr ui32 PhysicsUpdatesPerFrame = 100;
	static constexpr ui32 RendererDrawPerFrame = 100;

public:
	Benchmark2Class()
	{
		Log->Info("", "ECS multithreaded: %s\n", IsMTECS ? "yes" : "no");
		Log->Info("", "IsPhysicsFPSRestricted: %s\n", IsPhysicsFPSRestricted ? "yes" : "no");
		Log->Info("", "IsPhysicsUsingComponentChangedHints: %s\n", IsPhysicsUsingComponentChangedHints ? "yes" : "no");
		Log->Info("", "IsShuffleUpdatesOrder: %s\n", IsShuffleUpdatesOrder ? "yes" : "no");
		Log->Info("", "EntitiesToTest: %u\n", EntitiesToTest);
		Log->Info("", "PhysicsUpdatesPerFrame: %u\n", PhysicsUpdatesPerFrame);
		Log->Info("", "RendererDrawPerFrame: %u\n", RendererDrawPerFrame);

		auto idGenerator = EntityIDGenerator{};
		auto manager = SystemsManager::New(IsMTECS, Log);
		auto stream = make_unique<EntitiesStream>();

		auto before = TimeMoment::Now();
		GenerateScene(idGenerator, *manager, *stream);
		auto after = TimeMoment::Now();
		Log->Info("", "Generating scene took %.2lfs\n", (after - before).ToSec_f64());

		auto physicsPipeline = manager->CreatePipeline(IsPhysicsFPSRestricted ? optional(TimeSecondsFP64(1.0 / 60.0)) : nullopt, false);
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

				Log->Info("", "renderer %.1ffps (%.2lfms, %.2lfms), physics %.1ffps (%.2lfms, %.2lfms)\n", rfps, rendererSpent, rendererSpent / rfps, pfps, physicsSpent, physicsSpent / pfps);

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
		Log->Info("", "\nAverage FPS: %.2lf\n", avg / fpsHistory.size());
	}

    struct Physics : Component<Physics>
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

    struct MeshRenderer : Component<MeshRenderer>
    {
        array<char, 260> mesh;
    };

    struct Position : Component<Position>
    {
        Vector3 position;

        Position() = default;
        Position(const Vector3 &position) : position(position) {}
    };

    struct Rotation : Component<Rotation>
    {
        Quaternion rotation;

        Rotation() = default;
        Rotation(const Quaternion &rotation) : rotation(rotation) {}
    };

    struct MeshCollider : Component<MeshCollider>
    {
        bool isTrigger;
        array<char, 260> mesh;
    };

    struct PhysicsSystem : IndirectSystem<PhysicsSystem>
    {
        void Accept(Array<Position> &, Array<Rotation> &, const Array<MeshCollider> &, const Array<Physics> &);

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
            for (auto entry : stream.Enumerate<Position>())
            {
                _entities[entry.entityID].pos = entry.component.position;
            }
            for (auto entry : stream.Enumerate<Rotation>())
            {
                _entities[entry.entityID].rot = entry.component.rotation;
            }
            for (auto entry : stream.Enumerate<Physics>())
            {
                _entities[entry.entityID].properties = entry.component;
            }

			ASSUME(stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == Physics::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
			ASSUME(stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == Physics::GetTypeId() || stream.Type() == MeshCollider::GetTypeId());

			uiw size = _entities.size();

			for (const auto &entry : stream.Enumerate<Position>())
            {
				_entities.erase(entry.entityID);
            }
			for (const auto &entry : stream.Enumerate<Rotation>())
			{
				_entities.erase(entry.entityID);
			}
			for (const auto &entry : stream.Enumerate<Physics>())
			{
				_entities.erase(entry.entityID);
			}
			for (const auto &entry : stream.Enumerate<MeshCollider>())
			{
				_entities.erase(entry.entityID);
			}

			if (size != _entities.size())
			{
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

    struct RendererSystem : IndirectSystem<RendererSystem>
    {
        void Accept(const Array<Position> &, const Array<Rotation> &, const Array<MeshRenderer> &);

        struct Data
        {
            EntityID id;
            Vector3 pos;
            Quaternion rot;
        };

        virtual void Update(Environment &env) override
        {
			//std::this_thread::sleep_for(1ms);
            //ui32 count = 0;
            //for (auto it = _linear.begin(); count < RendererDrawPerFrame && it != _linear.end(); ++it, ++count)
            //{
            //}
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entry : stream)
            {
                Data data;
				data.id = entry.entityID;
                data.pos = entry.GetComponent<Position>().position;
                data.rot = entry.GetComponent<Rotation>().rotation;
				_entities[entry.entityID] = (ui32)_linear.size();
				_linear.emplace_back(data);
				_meshLinear.emplace_back(entry.GetComponent<MeshRenderer>());
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            for (auto &entry : stream)
            {
                Data data;
				data.id = entry.entityID;
                data.pos = entry.GetComponent<Position>().position;
                data.rot = entry.GetComponent<Rotation>().rotation;
				_entities[entry.entityID] = (ui32)_linear.size();
				_linear.emplace_back(data);
				_meshLinear.emplace_back(entry.GetComponent<MeshRenderer>());
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
			if (_linear.size() == _entities.size())
			{
				_linear.emplace_back();
			}

			uiw prevIndex = uiw_max;
			for (auto entry : stream.Enumerate<Position>())
			{
				uiw index = prevIndex + 1;
				Data *data = &_linear[index];
				if (data->id != entry.entityID)
				{
					index = _entities[entry.entityID];
					data = &_linear[index];
				}
				data->pos = entry.component.position;
				prevIndex = index;
			}

			prevIndex = uiw_max;
			for (auto entry : stream.Enumerate<Rotation>())
			{
				uiw index = prevIndex + 1;
				Data *data = &_linear[index];
				if (data->id != entry.entityID)
				{
					index = _entities[entry.entityID];
					data = &_linear[index];
				}
				data->rot = entry.component.rotation;
				prevIndex = index;
			}

            for (auto entry : stream.Enumerate<MeshRenderer>())
            {
				_meshLinear[_entities[entry.entityID]] = entry.component;
            }

			ASSUME(stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == MeshRenderer::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
			ASSUME(stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == MeshRenderer::GetTypeId());
			
			for (const auto &entry : stream.Enumerate<Position>())
			{
				RemoveEntity(entry.entityID);
			}
			for (const auto &entry : stream.Enumerate<Rotation>())
			{
				RemoveEntity(entry.entityID);
			}
			for (const auto &entry : stream.Enumerate<MeshRenderer>())
			{
				RemoveEntity(entry.entityID);
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
		vector<MeshRenderer> _meshLinear{};
    };

    static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
    {
		stream.HintTotal(EntitiesToTest);

		vector<EntityID> ids;
		if (IsShuffleUpdatesOrder)
		{
			TimeMoment before = TimeMoment::Now();
			ids.resize(EntitiesToTest);
			for (EntityID &id : ids)
			{
				id = entityIdGenerator.Generate();
			}
			auto pred = [](EntityID left, EntityID right)
			{
				return Hash::FNVHash<Hash::Precision::P32>(left.Hash()) < Hash::FNVHash<Hash::Precision::P32>(right.Hash());
			};
			std::sort(ids.begin(), ids.end(), pred);
			TimeMoment after = TimeMoment::Now();
			Log->Info("", "Generating entity ids tool %.2lfs\n", (after - before).ToSec_f64());
		}

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

			EntityID id;
			if (IsShuffleUpdatesOrder)
			{
				id = ids[index];
			}
			else
			{
				id = entityIdGenerator.Generate();
			}

            stream.AddEntity(id, move(entity));
        }
    }
};

void Benchmark2()
{
    StdLib::Initialization::Initialize({});
	Benchmark2Class test;
}