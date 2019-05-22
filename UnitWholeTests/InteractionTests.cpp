#include "PreHeader.hpp"
#include <stdio.h>
#include <tuple>
#include <set>
#include <queue>

using namespace ECSTest;

namespace
{
    constexpr ui32 EntitiesToAdd = 50;
    constexpr bool IsMultiThreadedECS = false;
    constexpr ui32 WaitForExecutedFrames = 150;

    class Pipeline
    {
        static constexpr ui32 _advanceLatency = 10;
        ui32 _advancePassed = 0;
        ui32 _phase = 0;

    public:
        ui32 Phase() const
        {
            return _phase;
        }

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

    COMPONENT(GeneratedComponent)
    {
        ui32 value;
        ui32 tagsCount;
    };

    COMPONENT(TempComponent)
    {};

    COMPONENT(GeneratorInfoComponent)
    {
        ui32 tagComponentsGenerated = 0;
        ui64 tagConnectionHash = 0;
        ui64 tagIDHash = 0;
    };

    COMPONENT(ConsumerInfoComponent)
    {
        ui32 entityAdded = 0;
        ui32 entityRemoved = 0;
        ui32 generatedComponentAdded = 0;
        ui32 generatedComponentChanged = 0;
        ui32 tempComponentRemoved = 0;
        ui32 tagComponentChanged = 0;
        ui32 tagComponentsReceived = 0;
        ui32 tagComponentsWithIdReceived = 0;
        ui64 tagConnectionHash = 0;
        ui64 tagIDHash = 0;
    };

    COMPONENT(OtherComponent)
    {
        ui64 value;
    };

    NONUNIQUE_COMPONENT(TagComponent)
    {
        enum class ConnectedTo
        {
            Russia, Germany, China, Netherlands
        };

        ConnectedTo connectedTo;
        optional<ComponentID> id;
    };

    TAG_COMPONENT(FilterTag);

    INDIRECT_SYSTEM(GeneratorSystem)
    {
        void Accept(Array<GeneratedComponent> *, SubtractiveComponent<ConsumerInfoComponent>, SubtractiveComponent<OtherComponent>, NonUnique<TagComponent> *, Array<GeneratorInfoComponent> *);

        virtual void Update(Environment &env) override
        {
            [this, &env]
            {
                if (_leftToGenerate)
                {
                    GeneratedComponent c;
                    c.value = 15;
                    c.tagsCount = 0;
                    auto &builder = env.messageBuilder.AddEntity(env.entityIdGenerator.Generate()).AddComponent(c).AddComponent(FilterTag{}).AddComponent(TempComponent{});
                    for (i32 index = 0, count = rand() % 4; index < count; ++index)
                    {
                        ++c.tagsCount;
                        TagComponent tag;
                        tag.connectedTo = TagComponent::ConnectedTo::Germany;
                        if (rand() % 2)
                        {
                            tag.id = env.componentIdGenerator.Generate();
                            builder.AddComponent(tag, *tag.id);
                            _info.tagIDHash ^= Hash::Integer((ui64)tag.id->ID());
                            _toTagChange.push({env.entityIdGenerator.LastGenerated(), tag});
                        }
                        else
                        {
                            builder.AddComponent(tag);
                        }
                        _info.tagConnectionHash ^= Hash::Integer((ui64)tag.connectedTo);
                        ++_info.tagComponentsGenerated;
                    }
                    c.value = 25;
                    env.messageBuilder.ComponentChanged(env.entityIdGenerator.LastGenerated(), c);
                    --_leftToGenerate;
                    _toComponentChange.push({env.entityIdGenerator.LastGenerated(), c.tagsCount});

                    if (_pipeline.Phase() == 0)
                    {
                        if (!_pipeline.Advance())
                        {
                            return;
                        }

                        env.logger.Message(LogLevels::Info, "Finished generating entities\n");
                    }
                }

                if (_toTagChange.size())
                {
                    TagComponent tag = _toTagChange.front().second;
                    ASSUME(tag.connectedTo == TagComponent::ConnectedTo::Germany);
                    tag.connectedTo = TagComponent::ConnectedTo::China;
                    env.messageBuilder.ComponentChanged(_toTagChange.front().first, tag, _toTagChange.front().second.id.value());
                    _toTagChange.pop();
                }

                if (_toComponentChange.size())
                {
                    GeneratedComponent c;
                    c.value = 35;
                    c.tagsCount = _toComponentChange.front().second;
                    env.messageBuilder.ComponentChanged(_toComponentChange.front().first, c);
                    _toComponentRemove.push(_toComponentChange.front().first);
                    _toComponentChange.pop();

                    if (_pipeline.Phase() == 1)
                    {
                        if (!_pipeline.Advance())
                        {
                            return;
                        }

                        env.logger.Message(LogLevels::Info, "Finished changing components\n");
                    }
                }

                if (_toComponentRemove.size())
                {
                    env.messageBuilder.RemoveComponent(_toComponentRemove.front(), TempComponent{});
                    _toEntityRemove.push(_toComponentRemove.front());
                    _toComponentRemove.pop();

                    if (_pipeline.Phase() == 2)
                    {
                        if (!_pipeline.Advance())
                        {
                            return;
                        }

                        env.logger.Message(LogLevels::Info, "Finished removing components\n");
                    }
                }

                if (_toEntityRemove.size())
                {
                    env.messageBuilder.RemoveEntity(_toEntityRemove.front());
                    _toEntityRemove.pop();
                }
            }();

            if (MemOps::Compare(&_info, &_infoPassed, sizeof(_info)))
            {
                env.messageBuilder.ComponentChanged(_infoID, _info);
                _infoPassed = _info;
            }
        }

        virtual void OnCreate(Environment &env) override
        {
            _infoID = env.entityIdGenerator.Generate();
            env.messageBuilder.AddEntity(_infoID).AddComponent(_info);
        }

        virtual void OnDestroy(Environment &env) override
        {
            env.messageBuilder.RemoveEntity(_infoID);
        }

    private:
        ui32 _leftToGenerate = EntitiesToAdd;
        std::queue<pair<EntityID, ui32>> _toComponentChange{};
        std::queue<pair<EntityID, TagComponent>> _toTagChange{};
        std::queue<EntityID> _toComponentRemove{};
        std::queue<EntityID> _toEntityRemove{};
        Pipeline _pipeline{};
        EntityID _infoID{};
        GeneratorInfoComponent _info{}, _infoPassed{};
    };

    INDIRECT_SYSTEM(ConsumerIndirectSystem)
    {
        void Accept(const Array<GeneratedComponent> &, const Array<TempComponent> *, const NonUnique<TagComponent> *, Array<ConsumerInfoComponent> *, RequiredComponent<FilterTag>);

        virtual void Update(Environment &env) override
        {
            if (MemOps::Compare(&_info, &_infoPassed, sizeof(_infoPassed)))
            {
                env.messageBuilder.ComponentChanged(_infoID, _info);
            }
            _infoPassed = _info;
        }

        virtual void OnCreate(Environment &env) override
        {
            _infoID = env.entityIdGenerator.Generate();
            env.messageBuilder.AddEntity(_infoID).AddComponent(_info);
        }

        virtual void OnDestroy(Environment &env) override
        {
            env.messageBuilder.RemoveEntity(_infoID);
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entity : stream)
            {
                auto c = entity.FindComponent<GeneratedComponent>();
                ASSUME(c);
                ++_info.entityAdded;
                ASSUME(c->value == 15);

                for (auto &attached : entity.components)
                {
                    if (attached.type == TagComponent::GetTypeId())
                    {
                        ASSUME(attached.id);
                        ASSUME(attached.isUnique == false);
                        ++_info.tagComponentsReceived;

                        auto &t = attached.Cast<TagComponent>();
                        _info.tagConnectionHash ^= Hash::Integer((ui64)t.connectedTo);

                        _info.tagIDHash ^= Hash::Integer((ui64)t.id.value_or(ComponentID(0)).ID());

                        if (t.id)
                        {
                            ++_info.tagComponentsWithIdReceived;
                        }
                    }
                    else if (attached.type == GeneratedComponent::GetTypeId())
                    {
                        ASSUME(attached.id.IsValid() == false);
                        ASSUME(attached.isUnique == true);
                    }
                }
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            SOFTBREAK;
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
            for (auto entity : stream.Enumerate<GeneratedComponent>())
            {
				const auto &c = entity.component;
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
                ++_info.generatedComponentChanged;
            }
            for (auto entity : stream.Enumerate<TagComponent>())
            {
				const auto &c = entity.component;
                ASSUME(c.connectedTo == TagComponent::ConnectedTo::Germany || c.connectedTo == TagComponent::ConnectedTo::China);
                ASSUME(!c.id || c.id.value() == entity.component.id);
                ++_info.tagComponentChanged;
            }

			ASSUME(stream.Type() == GeneratedComponent::GetTypeId() || stream.Type() == TagComponent::GetTypeId());
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
        {
            if (stream.Type() != TempComponent::GetTypeId())
            {
                SOFTBREAK;
                return;
            }

            for (const auto &entity : stream.Enumerate<TempComponent>())
            {
                auto it = _entityInfos.find(entity.entityID);
                ASSUME(it != _entityInfos.end());
                ASSUME(it->second.component != nullopt);
                ASSUME(it->second.component->value == 35);
                ASSUME(it->second.isEntityRemoved == false);
                it->second.component = nullopt;
                ++_info.tempComponentRemoved;
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
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

    DIRECT_SYSTEM(ConsumerDirectSystem)
    {
		void Accept(const Array<GeneratedComponent> &generatedComponents, Environment &env, const Array<EntityID> &ids, const NonUnique<TagComponent> *tags)
        {
			ASSUME(env.targetSystem == GetTypeId());
            ASSUME(ids.size() > 0);
            ASSUME(ids.size() <= EntitiesToAdd);
            ASSUME(generatedComponents.size() == ids.size());
            if (tags)
            {
                ASSUME(tags->components.size() == tags->ids.size());
                ASSUME(tags->stride > 0);
                ASSUME(tags->components.size() / tags->stride == ids.size());

                for (uiw index = 0; index < tags->components.size(); ++index)
                {
                    ASSUME(!tags->components[index].id || tags->components[index].id == tags->ids[index]);
                }
            }
            for (auto &c : generatedComponents)
            {
                ASSUME(c.value == 25 || c.value == 35);
                ui32 stride = tags ? tags->stride : 0;
                ASSUME(stride == c.tagsCount);
            }
            for (auto &id : ids)
            {
                ASSUME(id.Hash() <= env.entityIdGenerator.LastGenerated().Hash());
            }
        }
    };

    namespace
    {
        std::map<EntityID, OtherComponent> CurrentData{};
        ui64 ValueGenerator{};
    }

    INDIRECT_SYSTEM(OtherIndirectSystem)
    {
        void Accept(Array<OtherComponent> &);
		using IndirectSystem::ProcessMessages;

        virtual void Update(Environment &env) override
        {
			ASSUME(env.targetSystem == GetTypeId());

            if (_leftToGenerate)
            {
                OtherComponent component;
                component.value = ValueGenerator++;
                env.messageBuilder.AddEntity(env.entityIdGenerator.Generate()).AddComponent(component);
                CurrentData[env.entityIdGenerator.LastGenerated()] = component;
                _localData[env.entityIdGenerator.LastGenerated()] = component;
            }

            ASSUME(CurrentData.size() == _localData.size());

            for (auto &[key, value] : CurrentData)
            {
                ASSUME(_localData[key].value == value.value);
            }
        }

        virtual void OnCreate(Environment &env) override
        {}

        virtual void OnDestroy(Environment &env) override
        {}

        virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
        {
            for (auto &entry : stream)
            {
                CurrentData[entry.entityID] = entry.GetComponent<OtherComponent>();
                _localData[entry.entityID] = entry.GetComponent<OtherComponent>();
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
        {
            for (auto &entry : stream)
            {
                CurrentData[entry.entityID] = entry.added.Cast<OtherComponent>();
                _localData[entry.entityID] = entry.added.Cast<OtherComponent>();
            }
        }

        virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
        {
			ASSUME(stream.Type() == OtherComponent::GetTypeId());
            for (auto entry : stream.Enumerate<OtherComponent>())
            {
				CurrentData[entry.entityID] = entry.component;
				_localData[entry.entityID] = entry.component;
            }
        }

    private:
        ui32 _leftToGenerate = EntitiesToAdd;
        std::map<EntityID, OtherComponent> _localData{};
    };

    DIRECT_SYSTEM(EmptyDirectReadSystem)
    {
		void Accept(const Array<OtherComponent> &components, const Array<EntityID> &ids, Environment &env)
        {
			ASSUME(env.targetSystem == GetTypeId());

            for (uiw index = 0; index < components.size(); ++index)
            {
                auto &component = components[index];
                EntityID id = ids[index];
                ASSUME(CurrentData[id].value == component.value);
            }
        }
    };

    DIRECT_SYSTEM(EmptyDirectWriteSystem)
    {
		void Accept(Environment &env, Array<OtherComponent> &components, const Array<EntityID> &ids)
        {
			ASSUME(env.targetSystem == GetTypeId());

            for (uiw index = 0; index < components.size(); ++index)
            {
                auto &component = components[index];
                EntityID id = ids[index];
                ASSUME(CurrentData[id].value == component.value);
                component.value = ValueGenerator++;
                CurrentData[id] = component;
            }
        }
    };

    using namespace ECSTest;

    static void PrintStreamInfo(IEntitiesStream &stream, bool isFirstPass)
    {
        if (!isFirstPass)
        {
            printf("\n-------------\n\n");
        }

        ui32 tagComponentsSent = 0;
        ui32 tagComponentsReceived = 0;
        ui64 tagSentConnectionHash = 0;
        ui64 tagReceivedHash = 0;
        ui64 tagSentIDHash = 0;
        ui64 tagReceivedIDHash = 0;

        while (auto entity = stream.Next())
        {
            for (auto &c : entity->components)
            {
                if (c.type == ConsumerInfoComponent::GetTypeId())
                {
                    const auto &t = *(ConsumerInfoComponent *)c.data;
                    printf("consumer stats:\n");
                    printf("  componentAdded %u\n", t.generatedComponentAdded);
                    printf("  componentChanged %u", t.generatedComponentChanged);
                    printf("  componentRemoved %u", t.tempComponentRemoved);
                    printf("  entityAdded %u\n", t.entityAdded);
                    printf("  entityRemoved %u\n", t.entityRemoved);

                    ASSUME(t.generatedComponentAdded == 0);
                    ASSUME(t.generatedComponentChanged == EntitiesToAdd * 2);
                    ASSUME(t.tempComponentRemoved == EntitiesToAdd);
                    ASSUME(t.tagComponentChanged == t.tagComponentsWithIdReceived);
                    ASSUME(t.entityAdded == EntitiesToAdd);
                    ASSUME(t.entityRemoved == EntitiesToAdd);

                    tagComponentsReceived = t.tagComponentsReceived;
                    tagReceivedHash = t.tagConnectionHash;
                    tagReceivedIDHash = t.tagIDHash;
                }
                else if (c.type == GeneratorInfoComponent::GetTypeId())
                {
                    const auto &t = *(GeneratorInfoComponent *)c.data;
                    tagComponentsSent = t.tagComponentsGenerated;
                    tagSentConnectionHash = t.tagConnectionHash;
                    tagSentIDHash = t.tagIDHash;
                }
            }
        }

        ASSUME(tagComponentsReceived == tagComponentsSent);
        ASSUME(tagReceivedHash == tagSentConnectionHash);
        ASSUME(tagReceivedIDHash == tagSentIDHash);
    }
}

void InteractionTests()
{
    StdLib::Initialization::Initialize({});

    auto logger = make_shared<Logger<string_view, true>>();
    auto handle0 = logger->OnMessage(LogRecipient);

    auto manager = SystemsManager::New(IsMultiThreadedECS, logger);
    EntityIDGenerator entityIdGenerator;

    auto testPipeline0 = manager->CreatePipeline(/*5_ms*/nullopt, false);
    auto testPipeline1 = manager->CreatePipeline(/*6.5_ms*/nullopt, false);

    manager->Register<GeneratorSystem>(testPipeline0);
    manager->Register<EmptyDirectReadSystem>(testPipeline0);
    manager->Register<ConsumerIndirectSystem>(testPipeline1);
    manager->Register<ConsumerDirectSystem>(testPipeline1);
    manager->Register<EmptyDirectWriteSystem>(testPipeline1);
    manager->Register<OtherIndirectSystem>(testPipeline1);

    vector<WorkerThread> workers;
    if (IsMultiThreadedECS)
    {
        workers.resize(SystemInfo::LogicalCPUCores());
    }

    manager->Start(move(entityIdGenerator), move(workers));

    for (;;)
    {
        auto info0 = manager->GetPipelineInfo(testPipeline0);
        auto info1 = manager->GetPipelineInfo(testPipeline1);
        if (info0.executedTimes > WaitForExecutedFrames && info1.executedTimes > WaitForExecutedFrames)
        {
            break;
        }
        std::this_thread::yield();
    }

    manager->Pause(true);

    auto ecsstream = manager->StreamOut();

    PrintStreamInfo(*ecsstream, true);

    manager->Stop(true);

	CurrentData = {};
	ValueGenerator = 0;
}