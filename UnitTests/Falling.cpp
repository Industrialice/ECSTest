#include "PreHeader.hpp"

using namespace ECSTest;

class FallingClass
{
    static constexpr ui32 EntitiesToTest = 100;
	static constexpr bool IsUseDirectForFalling = false;
	static constexpr bool IsPreGenerateTransform = false;
	static constexpr bool IsMultiThreadedECS = false;

public:
	FallingClass()
	{
		auto stream = make_unique<EntitiesStream>();
		auto manager = SystemsManager::New(IsMultiThreadedECS, Log);
		EntityIDGenerator entityIdGenerator;

		GenerateScene(entityIdGenerator, *manager, *stream);

		auto testPipeline0 = manager->CreatePipeline(5_ms, false);
		auto testPipeline1 = manager->CreatePipeline(6.5_ms, false);

		manager->Register<TransformGeneratorSystem>(testPipeline0);
		manager->Register<TransformHeightFixerSystem>(testPipeline0);
		if (IsUseDirectForFalling)
		{
			manager->Register<TransformFallingDirectSystem>(testPipeline0);
		}
		else
		{
			manager->Register<TransformFallingIndirectSystem>(testPipeline0);
		}
		manager->Register<AverageHeightAnalyzerSystem>(testPipeline1);
		manager->Register<CooldownUpdater>(testPipeline1);

		vector<WorkerThread> workers;
		if (IsMultiThreadedECS)
		{
			workers.resize(SystemInfo::LogicalCPUCores());
		}

		manager->Start(move(entityIdGenerator), move(workers), move(stream));

		std::this_thread::sleep_for(2000ms);

		manager->Pause(true);

		auto ecsstream = manager->StreamOut();

		PrintStreamInfo(*ecsstream, true);

		manager->Resume();

		std::this_thread::sleep_for(1500ms);

		manager->Pause(true);

		ecsstream = manager->StreamOut();

		PrintStreamInfo(*ecsstream, false);

		manager->Stop(true);
	}

    struct Name : Component<Name>
    {
        array<char, 32> name;
    };

    struct Transform : Component<Transform>
    {
        Vector3 position;
    };

    struct AverageHeight : Component<AverageHeight>
    {
        f32 height;
        ui32 sources;
    };

    struct HeightFixerInfo : Component<HeightFixerInfo>
    {
        ui32 runTimes;
        ui32 heightsFixed;
    };

    struct NegativeHeightCooldown : Component<NegativeHeightCooldown>
    {
        f32 cooldown;
    };

    struct SpeedOfFall : Component<SpeedOfFall>
    {
        f32 speed;
    };

    static Vector3 GeneratePosition()
    {
        auto distr = []
        {
            // for some reason causes SIGILL on Android in Release
            std::random_device rd; // obtain a random number from hardware
            std::mt19937 eng(rd()); // seed the generator
            std::uniform_real_distribution<> distr(-50, 50); // define the range
            return static_cast<f32>(distr(eng));
            //return static_cast<f32>(rand() % 100 - 50);
        };
        return {distr(), distr(), distr()};
    }

    struct TransformGeneratorSystem : IndirectSystem<TransformGeneratorSystem>
    {
		void Accept(Array<Name> &, SubtractiveComponent<Transform>) {}
		using BaseIndirectSystem::ProcessMessages;

        virtual void Update(Environment &env) override
        {
            if (_entitiesToGenerate.empty())
            {
                return;
            }

            for (auto &id : _entitiesToGenerate)
            {
                Transform t;
                t.position = GeneratePosition();
                env.messageBuilder.AddComponent(id, t);
                if (rand() % 2)
                {
                    SpeedOfFall speed;
                    speed.speed = static_cast<f32>(rand() % 25);
                    env.messageBuilder.AddComponent(id, speed);
                }
            }
            _entitiesToGenerate.clear();

            env.logger.Info("Finished generating entities\n");
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entity : stream)
            {
                _entitiesToGenerate.push_back(entity.entityID);
                if (auto *name = entity.FindComponent<Name>(); name)
                {
                    string str = "Entity"s + to_string(entity.entityID.Hash());
                    ASSUME(str == string_view(name->name.data()));
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entity : stream)
            {
                auto it = std::find(_entitiesToGenerate.begin(), _entitiesToGenerate.end(), entity);
                ASSUME(it != _entitiesToGenerate.end());
                _entitiesToGenerate.erase(it);
            }
        }

    private:
        vector<EntityID> _entitiesToGenerate{};
    };

    struct TransformHeightFixerSystem : IndirectSystem<TransformHeightFixerSystem>
    {
		void Accept(Array<Transform> &, const Array<NegativeHeightCooldown> *, Array<HeightFixerInfo> *) {}

        virtual void Update(Environment &env) override
        {
            ++_info.runTimes;

            for (auto it = _entitiesToFix.begin(); it != _entitiesToFix.end(); )
            {
                if (_entitiesWithCooldown.find(it->first) == _entitiesWithCooldown.end())
                {
                    ++_info.heightsFixed;

                    env.messageBuilder.ComponentChanged(it->first, it->second);

                    NegativeHeightCooldown cooldown;
                    cooldown.cooldown = 0.1f;
                    env.messageBuilder.AddComponent(it->first, cooldown);
                    _entitiesWithCooldown.insert(it->first);

                    it = _entitiesToFix.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            env.messageBuilder.ComponentChanged(_infoId, _info);
        }

        virtual void OnCreate(Environment &env) override
        {
            ASSUME(_infoId.IsValid() == false);
			_infoId = env.messageBuilder.AddEntity();
            env.messageBuilder.AddComponent(_infoId, _info);
        }

        virtual void OnDestroy(Environment &env) override
        {
            ASSUME(_infoId);
            env.messageBuilder.RemoveEntity(_infoId);
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entity : stream)
            {
                auto t = entity.GetComponent<Transform>();
                if (t.position.y < 0)
                {
                    auto cooldown = entity.FindComponent<NegativeHeightCooldown>();
                    if (cooldown && cooldown->cooldown > 0)
                    {
                        _entitiesWithCooldown.insert(entity.entityID);
                    }
                    t.position.y = 100;
                    _entitiesToFix[entity.entityID] = t;
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            for (auto &entity : stream)
            {
                if (auto *name = entity.FindComponent<Name>(); name)
                {
                    string str = "Entity"s + to_string(entity.entityID.Hash());
                    ASSUME(str == string_view(name->name.data()));
                }

                if (stream.Type() == Transform::GetTypeId())
                {
                    Transform t = entity.added.Cast<Transform>();
                    if (t.position.y < 0)
                    {
                        t.position.y = 100;
                        _entitiesToFix[entity.entityID] = t;
                    }
                }
                else if (stream.Type() == NegativeHeightCooldown::GetTypeId())
                {
                    _entitiesWithCooldown.insert(entity.entityID);
                }
            }

			ASSUME(stream.Type() == Transform::GetTypeId() || stream.Type() == NegativeHeightCooldown::GetTypeId() || stream.Type() == HeightFixerInfo::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
			for (auto entity : stream.Enumerate<Transform>())
			{
				Transform t = entity.component;
				if (t.position.y < 0)
				{
					t.position.y = 100;
					_entitiesToFix[entity.entityID] = t;
				}
			}

			ASSUME(stream.Type() == Transform::GetTypeId() || stream.Type() == NegativeHeightCooldown::GetTypeId() || stream.Type() == HeightFixerInfo::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
			for (const auto &entity : stream.Enumerate<Transform>())
			{
				_entitiesToFix.erase(entity.entityID);
			}
			for (const auto &entity : stream.Enumerate<NegativeHeightCooldown>())
			{
				_entitiesWithCooldown.erase(entity.entityID);
			}

			ASSUME(stream.Type() == Transform::GetTypeId() || stream.Type() == NegativeHeightCooldown::GetTypeId() || stream.Type() == HeightFixerInfo::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entity : stream)
            {
                _entitiesToFix.erase(entity);
                _entitiesWithCooldown.erase(entity);
            }
        }

    private:
        std::map<EntityID, Transform> _entitiesToFix{};
        std::set<EntityID> _entitiesWithCooldown{};
        HeightFixerInfo _info{};
        EntityID _infoId{};
    };

    struct TransformFallingIndirectSystem : IndirectSystem<TransformFallingIndirectSystem>
    {
    private:
        struct Components
        {
            optional<Transform> transform{};
            optional<SpeedOfFall> speedOfFall{};
        };

	public:
		void Accept(Array<Transform> &, Array<SpeedOfFall> *) {}

        virtual void Update(Environment &env) override
        {
            for (auto &[entityID, components] : _entities)
            {
                ASSUME(components.transform);
                f32 speedOfFall = 50.0f;
                if (components.speedOfFall)
                {
                    speedOfFall = components.speedOfFall->speed;
                }
                components.transform->position.y -= env.timeSinceLastFrame * speedOfFall;
                env.messageBuilder.ComponentChanged(entityID, *components.transform);
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entity : stream)
            {
                auto &target = _entities[entity.entityID];
                target.transform = entity.GetComponent<Transform>();
                if (auto speedOfFall = entity.FindComponent<SpeedOfFall>(); speedOfFall)
                {
                    target.speedOfFall = *speedOfFall;
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            for (auto &entity : stream)
            {
                auto &target = _entities[entity.entityID];
                if (auto transform = entity.added.TryCast<Transform>(); transform)
                {
                    target.transform = *transform;
                }
                else if (auto speedOfFall = entity.added.TryCast<SpeedOfFall>(); speedOfFall)
                {
                    target.speedOfFall = *speedOfFall;
                }
                else
                {
                    SOFTBREAK;
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
			for (auto entity : stream.Enumerate<Transform>())
			{
				auto &target = _entities[entity.entityID];
				target.transform = entity.component;
			}
			for (auto entity : stream.Enumerate<SpeedOfFall>())
			{
				auto &target = _entities[entity.entityID];
				target.speedOfFall = entity.component;
			}

			ASSUME(stream.Type() == Transform::GetTypeId() || stream.Type() == SpeedOfFall::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            for (const auto &entity : stream.Enumerate<Transform>())
            {
				_entities.erase(entity.entityID);
            }
			for (const auto &entity : stream.Enumerate<SpeedOfFall>())
			{
				_entities[entity.entityID].speedOfFall = nullopt;
			}

			ASSUME(stream.Type() == Transform::GetTypeId() || stream.Type() == SpeedOfFall::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entity : stream)
            {
                _entities.erase(entity);
            }
        }

    private:
        std::map<EntityID, Components> _entities{};
    };

    struct TransformFallingDirectSystem : DirectSystem<TransformFallingDirectSystem>
    {
		void Accept(Array<Transform> &transforms, Array<SpeedOfFall> *speeds, Environment &env)
        {
            for (uiw index = 0; index < transforms.size(); ++index)
            {
                auto &t = transforms[index];
                f32 speedOfFall = 50.0f;
                if (speeds)
                {
                    speedOfFall = speeds->at(index).speed;
                }
                t.position.y -= env.timeSinceLastFrame * speedOfFall;
            }
        }
    };

    struct AverageHeightAnalyzerSystem : IndirectSystem<AverageHeightAnalyzerSystem>
    {
		void Accept(const Array<Transform> &, Array<AverageHeight> *) {}

        virtual void Update(Environment &env) override
        {
            if (!_isChanged)
            {
                return;
            }

            f64 sum = 0;
            for (auto &[id, y] : _entities)
            {
                sum += y;
            }
            sum /= _entities.size();

            AverageHeight h;
            h.height = static_cast<f32>(sum);
            h.sources = static_cast<ui32>(_entities.size());
            env.messageBuilder.ComponentChanged(_entityID, h);

            _isChanged = false;
        }

        virtual void OnCreate(Environment &env) override
        {
            ASSUME(_entityID.IsValid() == false);
            AverageHeight h;
            h.height = 0;
            h.sources = 0;
            _entityID = env.messageBuilder.AddEntity();
            env.messageBuilder.AddComponent(_entityID, h);
        }

        virtual void OnDestroy(Environment &env) override
        {
            ASSUME(_entityID);
            env.messageBuilder.RemoveEntity(_entityID);
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entity : stream)
            {
                _entities[entity.entityID] = entity.FindComponent<Transform>()->position.y;
            }
            _isChanged = true;
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            for (auto &entity : stream)
            {
                if (stream.Type() == Transform::GetTypeId())
                {
                    _entities[entity.entityID] = entity.added.Cast<Transform>().position.y;
                    _isChanged = true;
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
			ASSUME(stream.Type() == Transform::GetTypeId());
            for (auto entity : stream.Enumerate<Transform>())
            {
                _entities[entity.entityID] = entity.component.position.y;
            }
            _isChanged = true;
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
			ASSUME(stream.Type() == Transform::GetTypeId());
            for (const auto &entity : stream.Enumerate<Transform>())
            {
                _entities.erase(entity.entityID);
            }
            _isChanged = true;
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entity : stream)
            {
                _entities.erase(entity);
            }
            _isChanged = true;
        }

    private:
        std::map<EntityID, f32> _entities{};
        bool _isChanged = false;
        EntityID _entityID{};
    };

    struct CooldownUpdater : IndirectSystem<CooldownUpdater>
    {
		void Accept(Array<NegativeHeightCooldown> &) {}

        virtual void Update(Environment &env) override
        {
            for (auto it = _cooldowns.begin(); it != _cooldowns.end(); )
            {
                auto &[id, component] = *it;

                component.cooldown -= env.timeSinceLastFrame;
                if (component.cooldown <= 0)
                {
                    env.messageBuilder.RemoveComponent(id, component);
                    it = _cooldowns.erase(it);
                }
                else
                {
                    env.messageBuilder.ComponentChanged(id, component);
                    ++it;
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entity : stream)
            {
                _cooldowns[entity.entityID] = entity.GetComponent<NegativeHeightCooldown>();
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            for (auto &entity : stream)
            {
                _cooldowns[entity.entityID] = entity.added.Cast<NegativeHeightCooldown>();
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
            SOFTBREAK;
            for (auto entity : stream.Enumerate<NegativeHeightCooldown>())
            {
                _cooldowns[entity.entityID] = entity.component;
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            SOFTBREAK;
            for (const auto &entity : stream.Enumerate<NegativeHeightCooldown>())
            {
                _cooldowns.erase(entity.entityID);
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
        {
            for (auto &entity : stream)
            {
                _cooldowns.erase(entity);
            }
        }

    private:
        std::map<EntityID, NegativeHeightCooldown> _cooldowns{};
    };

    static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
    {
		stream.HintTotal(EntitiesToTest);

        for (uiw index = 0; index < EntitiesToTest; ++index)
        {
            EntitiesStream::EntityData entity;

            if (IsPreGenerateTransform)
            {
                Transform t;
                t.position = GeneratePosition();
                entity.AddComponent(t);
                if (rand() % 2)
                {
                    SpeedOfFall speed;
                    speed.speed = static_cast<f32>(rand() % 25);
                    entity.AddComponent(speed);
                }
            }

            EntityID id = entityIdGenerator.Generate();

            string name = "Entity"s + std::to_string(id.Hash());
            Name n;
            MemOps::Copy(n.name.data(), name.c_str(), std::min(name.size() + 1, n.name.size()));
			n.name.back() = '\0';
            entity.AddComponent(n);

            stream.AddEntity(id, move(entity));
        }
    }

    static void PrintStreamInfo(IEntitiesStream &stream, bool isFirstPass)
    {
        if (!isFirstPass)
        {
			Log->Info("", "\n-------------\n\n");
        }

        std::map<TypeId, ui32> componentCounts{};

        while (auto entity = stream.Next())
        {
            for (auto &c : entity->components)
            {
                componentCounts[c.type]++;

                if (c.type == AverageHeight::GetTypeId())
                {
                    AverageHeight a;
                    MemOps::Copy(reinterpret_cast<byte *>(&a), c.data, sizeof(AverageHeight));
					Log->Info("", "average height %f based on %u sources\n", a.height, a.sources);
                }
                else if (c.type == HeightFixerInfo::GetTypeId())
                {
                    HeightFixerInfo i;
                    MemOps::Copy(reinterpret_cast<byte *>(&i), c.data, sizeof(HeightFixerInfo));
					Log->Info("", "height fixer run %u times, fixed %u heights\n", i.runTimes, i.heightsFixed);
                }
            }
        }

		Log->Info("", "\n");
        for (auto &[id, count] : componentCounts)
        {
            if (id == Transform::GetTypeId())
            {
                ASSUME(count == EntitiesToTest);
            }
            else if (id == Name::GetTypeId())
            {
                ASSUME(count == EntitiesToTest);
            }
            else if (id == AverageHeight::GetTypeId())
            {
                ASSUME(count == 1);
            }
            else if (id == HeightFixerInfo::GetTypeId())
            {
                ASSUME(count == 1);
            }

			Log->Info("", "%s count %u\n", id.Name(), count);
        }
    }
};

void Falling()
{
    StdLib::Initialization::Initialize({});
	FallingClass test;
}