#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>
#include <set>
#include <queue>

using namespace ECSTest;

namespace
{
    constexpr ui32 EntitiesToAdd = 50;
    constexpr bool IsMultiThreadedECS = false;
}

COMPONENT(GeneratedComponent)
{
    ui32 value;
};

COMPONENT(ConsumerInfoComponent)
{
    ui32 entityAdded = 0;
    ui32 entityRemoved = 0;
    ui32 componentAdded = 0;
    ui32 componentChanged = 0;
    ui32 componentRemoved = 0;
};

INDIRECT_SYSTEM(GeneratorSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(SubtractiveComponent<GeneratedComponent>, SubtractiveComponent<ConsumerInfoComponent>)
    {
        if (_leftToGenerate)
        {
            GeneratedComponent c;
            c.value = 15;
            env.messageBuilder.AddEntity(env.idGenerator.Generate()).AddComponent(c);
            c.value = 25;
            env.messageBuilder.ComponentChanged(env.idGenerator.LastGenerated(), c);
            --_leftToGenerate;
            _toComponentChange.push(env.idGenerator.LastGenerated());
            
            if (!_pipeline.Advance())
            {
                return;
            }
        }

        if (_toComponentChange.size())
        {
            GeneratedComponent c;
            c.value = 35;
            env.messageBuilder.ComponentChanged(_toComponentChange.front(), c);
            _toComponentRemove.push(_toComponentChange.front());
            _toComponentChange.pop();

            if (!_pipeline.Advance())
            {
                return;
            }
        }

        if (_toComponentRemove.size())
        {
            env.messageBuilder.ComponentRemoved(_toComponentRemove.front(), GeneratedComponent{});
            _toEntityRemove.push(_toComponentRemove.front());
            _toComponentRemove.pop();

            if (!_pipeline.Advance())
            {
                return;
            }
        }

        if (_toEntityRemove.size())
        {
            env.messageBuilder.RemoveEntity(_toEntityRemove.front());
            _toEntityRemove.pop();
        }
    }

private:
    class Pipeline
    {
        static constexpr ui32 _advanceLatency = 10;
        ui32 _advancePassed = 0;
        ui32 _phase = 0;

    public:
        bool Advance()
        {
            if (_phase != 3)
            {
                ++_advancePassed;
                if (_advancePassed < _advanceLatency)
                {
                    return false;
                }
                _advancePassed = 0;
                ++_phase;
            }
            return true;
        }
    };

    ui32 _leftToGenerate = EntitiesToAdd;
    std::queue<EntityID> _toComponentChange{};
    std::queue<EntityID> _toComponentRemove{};
    std::queue<EntityID> _toEntityRemove{};
    Pipeline _pipeline{};
};

void GeneratorSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    SOFTBREAK;
}

void GeneratorSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{
    SOFTBREAK;
}

void GeneratorSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
    SOFTBREAK;
}

void GeneratorSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
{
    SOFTBREAK;
}

void GeneratorSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    SOFTBREAK;
}

INDIRECT_SYSTEM(ConsumerSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<GeneratedComponent> &)
    {
        if (MemOps::Compare(&_info, &_infoPassed, sizeof(_infoPassed)))
        {
            env.messageBuilder.ComponentChanged(_infoID, _info);
        }
        _infoPassed = _info;
    }

    virtual void OnCreate(Environment &env) override 
    {
        _infoID = env.idGenerator.Generate();
        env.messageBuilder.AddEntity(_infoID).AddComponent(_info);
    }

    virtual void OnDestroy(Environment &env) override
    {
        env.messageBuilder.RemoveEntity(_infoID);
    }

private:
    struct EntityInfo
    {
        optional<GeneratedComponent> component;
        bool isEntityRemoved = false;
    };

    EntityID _infoID{};
    ConsumerInfoComponent _info{}, _infoPassed{};
    std::map<EntityID, EntityInfo> _entityInfos{};
};

void ConsumerSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entity : stream)
    {
        if (auto c = entity.FindComponent<GeneratedComponent>(); c)
        {
            ++_info.entityAdded;
            ASSUME(c->value == 15);
        }
    }
}

void ConsumerSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{
    SOFTBREAK;
}

void ConsumerSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
    ASSUME(stream.Type() == GeneratedComponent::GetTypeId());

    for (auto &entity : stream)
    {
        const auto &c = entity.component.Cast<GeneratedComponent>();
        auto it = _entityInfos.find(entity.entityID);
        if (it == _entityInfos.end())
        {
            ASSUME(c.value == 25);
            _entityInfos[entity.entityID] = {c};
        }
        else
        {
            ASSUME(it->second.component != nullopt);
            ASSUME(it->second.component->value == 25);
            ASSUME(it->second.isEntityRemoved == false);
            ASSUME(c.value == 35);
            it->second.component = c;
        }
        ++_info.componentChanged;
    }
}

void ConsumerSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
{
    ASSUME(stream.Type() == GeneratedComponent::GetTypeId());

    for (auto &entity : stream)
    {
        auto it = _entityInfos.find(entity.entityID);
        ASSUME(it != _entityInfos.end());
        ASSUME(it->second.component != nullopt);
        ASSUME(it->second.component->value == 35);
        ASSUME(it->second.isEntityRemoved == false);
        it->second.component = nullopt;
        ++_info.componentRemoved;
    }
}

void ConsumerSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entity : stream)
    {
        auto it = _entityInfos.find(entity);
        ASSUME(it != _entityInfos.end());
        ASSUME(it->second.component == nullopt);
        ASSUME(it->second.isEntityRemoved == false);
        it->second.isEntityRemoved = true;
        ++_info.entityRemoved;
    }
}

using namespace ECSTest;

class TestEntities : public EntitiesStream
{
public:
    struct PreStreamedEntity
    {
        StreamedEntity streamed;
        vector<ComponentDesc> descs;
        vector<unique_ptr<ui8[]>> componentsData;

        PreStreamedEntity() = default;
        PreStreamedEntity(PreStreamedEntity &&) = default;
        PreStreamedEntity &operator = (PreStreamedEntity &&) = default;
    };

private:
    vector<PreStreamedEntity> _entities{};
    uiw _currentEntity{};

public:
    [[nodiscard]] virtual optional<StreamedEntity> Next() override
    {
        if (_currentEntity < _entities.size())
        {
            uiw index = _currentEntity++;
            return _entities[index].streamed;
        }
        return {};
    }

    void AddEntity(EntityID id, PreStreamedEntity &&entity)
    {
        _entities.emplace_back(move(entity));
        _entities.back().streamed.components = ToArray(_entities.back().descs);
        _entities.back().streamed.entityId = id;
    }
};

template <typename T> void StreamComponent(const T &component, TestEntities::PreStreamedEntity &preStreamed)
{
    EntitiesStream::ComponentDesc desc;
    auto componentData = make_unique<ui8[]>(sizeof(T));
    memcpy(componentData.get(), &component, sizeof(T));
    desc.alignmentOf = alignof(T);
    desc.isUnique = T::IsUnique();
    desc.sizeOf = sizeof(T);
    desc.type = T::GetTypeId();
    desc.data = componentData.get();
    preStreamed.componentsData.emplace_back(move(componentData));
    preStreamed.descs.emplace_back(desc);
}

static void GenerateScene(EntityIDGenerator &idGenerator, SystemsManager &manager, TestEntities &stream)
{
    //for (uiw index = 0; index < EntitiesToTest; ++index)
    //{
    //    TestEntities::PreStreamedEntity entity;

    //    if (IsPreGenerateTransform)
    //    {
    //        Transform t;
    //        t.position.x = rand() / (f32)RAND_MAX * 100 - 50;
    //        t.position.y = rand() / (f32)RAND_MAX * 100 - 50;
    //        t.position.z = rand() / (f32)RAND_MAX * 100 - 50;
    //        StreamComponent(t, entity);
    //    }

    //    string name = "Entity"s + std::to_string(index);
    //    Name n;
    //    strcpy_s(n.name.data(), n.name.size(), name.c_str());
    //    StreamComponent(n, entity);

    //    stream.AddEntity(idGenerator.Generate(), move(entity));
    //}
}

static void PrintStreamInfo(EntitiesStream &stream, bool isFirstPass)
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

            if (c.type == ConsumerInfoComponent::GetTypeId())
            {
                const auto &t = *(ConsumerInfoComponent *)c.data;
                printf("consumer stats:\n");
                printf("  componentAdded %u\n", t.componentAdded);
                printf("  componentChanged %u", t.componentChanged);
                printf("  componentRemoved %u", t.componentRemoved);
                printf("  entityAdded %u\n", t.entityAdded);
                printf("  entityRemoved %u\n", t.entityRemoved);

                ASSUME(t.componentAdded == 0);
                ASSUME(t.componentChanged == EntitiesToAdd * 2);
                ASSUME(t.componentRemoved == EntitiesToAdd);
                ASSUME(t.entityAdded == EntitiesToAdd);
                ASSUME(t.entityRemoved == EntitiesToAdd);
            }
        }
    }
}

int main()
{
    StdLib::Initialization::Initialize({});

    auto stream = make_unique<TestEntities>();
    auto manager = SystemsManager::New(IsMultiThreadedECS);
    EntityIDGenerator idGenerator;

    GenerateScene(idGenerator, *manager, *stream);

    auto testPipelineGroup0 = manager->CreatePipelineGroup(5_ms, false);
    auto testPipelineGroup1 = manager->CreatePipelineGroup(6.5_ms, false);

    manager->Register(make_unique<GeneratorSystem>(), testPipelineGroup0);
    manager->Register(make_unique<ConsumerSystem>(), testPipelineGroup1);

    vector<WorkerThread> workers;
    if (IsMultiThreadedECS)
    {
        workers.resize(SystemInfo::LogicalCPUCores());
    }

    vector<unique_ptr<EntitiesStream>> streams;
    streams.push_back(move(stream));
    manager->Start(move(idGenerator), move(workers), move(streams));

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

    system("pause");
}