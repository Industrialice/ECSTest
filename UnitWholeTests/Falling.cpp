#include "PreHeader.hpp"
#include <stdio.h>
#include <tuple>
#include <set>
#include <random>

using namespace ECSTest;

namespace
{
    constexpr ui32 EntitiesToTest = 100;
    constexpr bool IsUseDirectForFalling = false;
    constexpr bool IsPreGenerateTransform = false;
    constexpr bool IsMultiThreadedECS = false;

    COMPONENT(Name)
    {
        array<char, 32> name;
    };

    COMPONENT(Transform)
    {
        Vector3 position;
    };

    COMPONENT(AverageHeight)
    {
        f32 height;
        ui32 sources;
    };

    COMPONENT(HeightFixerInfo)
    {
        ui32 runTimes;
        ui32 heightsFixed;
    };

    COMPONENT(NegativeHeightCooldown)
    {
        f32 cooldown;
    };

    COMPONENT(SpeedOfFall)
    {
        f32 speed;
    };

    static Vector3 GeneratePosition()
    {
        std::random_device rd; // obtain a random number from hardware
        std::mt19937 eng(rd()); // seed the generator
        std::uniform_real_distribution<> distr(-50, 50); // define the range
        return {(f32)distr(eng), (f32)distr(eng), (f32)distr(eng)};
    }

    INDIRECT_SYSTEM(TransformGeneratorSystem)
    {
        INDIRECT_ACCEPT_COMPONENTS(Array<Name> &, SubtractiveComponent<Transform>);

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
                    speed.speed = (f32)(rand() % 25);
                    env.messageBuilder.AddComponent(id, speed);
                }
            }
            _entitiesToGenerate.clear();

            env.logger.Message(LogLevels::Info, "Finished generating entities\n");
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

    INDIRECT_SYSTEM(TransformHeightFixerSystem)
    {
        INDIRECT_ACCEPT_COMPONENTS(Array<Transform> &, const Array<NegativeHeightCooldown> *, Array<HeightFixerInfo> *);

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
            _infoId = env.entityIdGenerator.Generate();
            env.messageBuilder.AddEntity(_infoId).AddComponent(_info);
        }

        virtual void OnDestroy(Environment &env) override
        {
            ASSUME(_infoId.IsValid());
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
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
            for (auto &entity : stream.Enumerate())
            {
                if (entity.component.type == Transform::GetTypeId())
                {
                    Transform t = *(Transform *)entity.component.data;
                    if (t.position.y < 0)
                    {
                        t.position.y = 100;
                        _entitiesToFix[entity.entityID] = t;
                    }
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            for (auto &entity : stream)
            {
                if (stream.Type() == Transform::GetTypeId())
                {
                    _entitiesToFix.erase(entity.entityID);
                }
                else if (stream.Type() == NegativeHeightCooldown::GetTypeId())
                {
                    _entitiesWithCooldown.erase(entity.entityID);
                }
            }
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

    INDIRECT_SYSTEM(TransformFallingIndirectSystem)
    {
    private:
        struct Components
        {
            optional<Transform> transform{};
            optional<SpeedOfFall> speedOfFall{};
        };

        INDIRECT_ACCEPT_COMPONENTS(Array<Transform> &, Array<SpeedOfFall> *);

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
            for (auto &entity : stream.Enumerate())
            {
                auto &target = _entities[entity.entityID];
                if (auto transform = entity.component.TryCast<Transform>(); transform)
                {
                    target.transform = *transform;
                }
                else if (auto speedOfFall = entity.component.TryCast<SpeedOfFall>(); speedOfFall)
                {
                    target.speedOfFall = *speedOfFall;
                }
                else
                {
                    SOFTBREAK;
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            for (auto &entity : stream)
            {
                if (stream.Type() == Transform::GetTypeId())
                {
                    _entities.erase(entity.entityID);
                }
                else if (stream.Type() == SpeedOfFall::GetTypeId())
                {
                    _entities[entity.entityID].speedOfFall = nullopt;
                }
                else
                {
                    SOFTBREAK;
                }
            }
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

    DIRECT_SYSTEM(TransformFallingDirectSystem)
    {
        DIRECT_ACCEPT_COMPONENTS(Array<Transform> &transforms, Array<SpeedOfFall> *speeds)
        {
            for (uiw index = 0; index < transforms.size(); ++index)
            {
                auto &t = transforms[index];
                f32 speedOfFall = 50.0f;
                if (speeds)
                {
                    speedOfFall = speeds->operator[](index).speed;
                }
                t.position.y -= env.timeSinceLastFrame * speedOfFall;
            }
        }
    };

    INDIRECT_SYSTEM(AverageHeightAnalyzerSystem)
    {
        INDIRECT_ACCEPT_COMPONENTS(const Array<Transform> &, Array<AverageHeight> *);

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
            h.height = (f32)sum;
            h.sources = (ui32)_entities.size();
            env.messageBuilder.ComponentChanged(_entityID, h);

            _isChanged = false;
        }

        virtual void OnCreate(Environment &env) override
        {
            ASSUME(_entityID.IsValid() == false);
            AverageHeight h;
            h.height = 0;
            h.sources = 0;
            _entityID = env.entityIdGenerator.Generate();
            env.messageBuilder.AddEntity(_entityID).AddComponent(h);
        }

        virtual void OnDestroy(Environment &env) override
        {
            ASSUME(_entityID.IsValid());
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
            for (auto &entity : stream.Enumerate())
            {
                _entities[entity.entityID] = entity.component.Cast<Transform>().position.y;
            }
            _isChanged = true;
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            for (auto &entity : stream)
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

    INDIRECT_SYSTEM(CooldownUpdater)
    {
        INDIRECT_ACCEPT_COMPONENTS(Array<NegativeHeightCooldown> &);

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
            for (auto &entity : stream.Enumerate())
            {
                _cooldowns[entity.entityID] = entity.component.Cast<NegativeHeightCooldown>();
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            SOFTBREAK;
            for (auto &entity : stream)
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

    using namespace ECSTest;

    static void GenerateScene(EntityIDGenerator &entityIdGenerator, SystemsManager &manager, EntitiesStream &stream)
    {
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
                    speed.speed = (f32)(rand() % 25);
                    entity.AddComponent(speed);
                }
            }

            EntityID id = entityIdGenerator.Generate();

            string name = "Entity"s + std::to_string(id.Hash());
            Name n;
            strcpy_s(n.name.data(), n.name.size(), name.c_str());
            entity.AddComponent(n);

            stream.AddEntity(id, move(entity));
        }
    }

    static void PrintStreamInfo(IEntitiesStream &stream, bool isFirstPass)
    {
        if (!isFirstPass)
        {
            printf("\n-------------\n\n");
        }

        std::map<StableTypeId, ui32> componentCounts{};

        while (auto entity = stream.Next())
        {
            for (auto &c : entity->components)
            {
                componentCounts[c.type]++;

                if (c.type == AverageHeight::GetTypeId())
                {
                    auto a = *(AverageHeight *)c.data;
                    printf("average height %f based on %u sources\n", a.height, a.sources);
                }
                else if (c.type == HeightFixerInfo::GetTypeId())
                {
                    auto i = *(HeightFixerInfo *)c.data;
                    printf("height fixer run %u times, fixed %u heights\n", i.runTimes, i.heightsFixed);
                }
            }
        }

        printf("\n");
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

            printf("%s count %u\n", id.Name(), count);
        }
    }
}

void Falling()
{
    StdLib::Initialization::Initialize({});

    auto logger = make_shared<Logger<string_view, true>>();
    auto handle0 = logger->OnMessage(LogRecipient);
    
    auto stream = make_unique<EntitiesStream>();
    auto manager = SystemsManager::New(IsMultiThreadedECS, logger);
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