#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>
#include <set>

using namespace ECSTest;

namespace
{
    constexpr ui32 EntitiesToTest = 100;
}

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

INDIRECT_SYSTEM(TransformGeneratorSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<Name> &, SubtractiveComponent<Transform>);

private:
    vector<EntityID> _entitiesToGenerate{};
};

void TransformGeneratorSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entity : stream)
    {
        _entitiesToGenerate.push_back(entity.entityID);
    }
}

void TransformGeneratorSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{
}

void TransformGeneratorSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{}

void TransformGeneratorSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
{}

void TransformGeneratorSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entity : stream)
    {
        auto it = std::find(_entitiesToGenerate.begin(), _entitiesToGenerate.end(), entity);
        ASSUME(it != _entitiesToGenerate.end());
        _entitiesToGenerate.erase(it);
    }
}

void TransformGeneratorSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{
    for (auto &id : _entitiesToGenerate)
    {
        Transform t;
        t.position.x = rand() / (f32)RAND_MAX * 100 - 50;
        t.position.y = rand() / (f32)RAND_MAX * 100 - 50;
        t.position.z = rand() / (f32)RAND_MAX * 100 - 50;
        messageBuilder.ComponentAdded(id, t);
    }
    _entitiesToGenerate.clear();
}

INDIRECT_SYSTEM(TransformHeightFixerSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<Transform> &, Array<NegativeHeightCooldown> *cooldowns);

private:
	std::map<EntityID, Transform> _entitiesToFix{};
    std::set<EntityID> _entitiesWithCooldown{};
    HeightFixerInfo _info{};
    EntityID _infoId{};
};

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entity : stream)
    {
        auto t = *entity.FindComponent<Transform>();
        if (t.position.y < 0)
        {
            auto cooldown = entity.FindComponent<NegativeHeightCooldown>();
            if (cooldown == nullptr || cooldown->cooldown <= 0)
            {
                t.position.y = 100;
            }
            else
            {
                _entitiesWithCooldown.insert(entity.entityID);
            }
            _entitiesToFix[entity.entityID] = t;
        }
    }
}

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{
    for (auto &entity : stream)
    {
        if (entity.component.type == Transform::GetTypeId())
        {
            Transform t = *(Transform *)entity.component.data;
            if (t.position.y < 0)
            {
                if (_entitiesWithCooldown.find(entity.entityID) == _entitiesWithCooldown.end())
                {
                    t.position.y = 100;
                }
                _entitiesToFix[entity.entityID] = t;
            }
        }
        else if (entity.component.type == NegativeHeightCooldown::GetTypeId())
        {
            _entitiesWithCooldown.insert(entity.entityID);
        }
    }
}

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
	for (auto &entity : stream)
	{
        if (entity.component.type == Transform::GetTypeId())
        {
            Transform t = *(Transform *)entity.component.data;
            if (t.position.y < 0)
            {
                t.position.y = 100;
                if (_entitiesWithCooldown.find(entity.entityID) == _entitiesWithCooldown.end())
                {
                    _entitiesToFix[entity.entityID] = t;
                }
            }
        }
	}
}

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
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

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entity : stream)
    {
        _entitiesToFix.erase(entity);
        _entitiesWithCooldown.erase(entity);
    }
}

void TransformHeightFixerSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{
    ++_info.runTimes;

    for (auto it = _entitiesToFix.begin(); it != _entitiesToFix.end(); )
	{
        if (_entitiesWithCooldown.find(it->first) == _entitiesWithCooldown.end())
        {
            ++_info.heightsFixed;

            messageBuilder.ComponentChanged(it->first, it->second);

            NegativeHeightCooldown cooldown;
            cooldown.cooldown = 0.1f;
            messageBuilder.ComponentAdded(it->first, cooldown);
            _entitiesWithCooldown.insert(it->first);

            it = _entitiesToFix.erase(it);
        }
        else
        {
            ++it;
        }
	}

    if (_infoId.IsValid())
    {
        messageBuilder.ComponentChanged(_infoId, _info);
    }
    else
    {
        _infoId = env.idGenerator.Generate();
        messageBuilder.EntityAdded(_infoId).AddComponent(_info);
    }
}

INDIRECT_SYSTEM(TransformFallingSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(Array<Transform> &);

private:
	std::map<EntityID, Transform> _entities{};
};

void TransformFallingSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
	for (auto &entity : stream)
	{
        _entities[entity.entityID] = *entity.FindComponent<Transform>();
	}
}

void TransformFallingSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{
    for (auto &entity : stream)
    {
        if (entity.component.type == Transform::GetTypeId())
        {
            _entities[entity.entityID] = *(Transform *)entity.component.data;
        }
    }
}

void TransformFallingSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
	for (auto &entity : stream)
	{
		_entities[entity.entityID] = *(Transform *)entity.component.data;
	}
}

void TransformFallingSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
{
    for (auto &entity : stream)
    {
        _entities.erase(entity.entityID);
    }
}

void TransformFallingSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
	for (auto &entity : stream)
	{
		_entities.erase(entity);
	}
}

void TransformFallingSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{
	for (auto &[entityID, component] : _entities)
	{
		component.position.y -= env.timeSinceLastFrame * 50.0f;
		messageBuilder.ComponentChanged(entityID, component);
	}
}

INDIRECT_SYSTEM(AverageHeightAnalyzerSystem)
{
    INDIRECT_ACCEPT_COMPONENTS(const Array<Transform> &);

private:
	std::map<EntityID, f32> _entities{};
	bool _isChanged = false;
	EntityID _entityID{};
};

void AverageHeightAnalyzerSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
	for (auto &entity : stream)
	{
        _entities[entity.entityID] = entity.FindComponent<Transform>()->position.y;
	}
	_isChanged = true;
}

void AverageHeightAnalyzerSystem::ProcessMessages(const MessageStreamComponentAdded &stream)
{
    for (auto &entity : stream)
    {
        if (entity.component.type == Transform::GetTypeId())
        {
            _entities[entity.entityID] = (*(Transform *)entity.component.data).position.y;
            _isChanged = true;
        }
    }
}

void AverageHeightAnalyzerSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
	for (auto &entity : stream)
	{
		_entities[entity.entityID] = (*(Transform *)entity.component.data).position.y;
	}
	_isChanged = true;
}

void AverageHeightAnalyzerSystem::ProcessMessages(const MessageStreamComponentRemoved &stream)
{
    for (auto &entity : stream)
    {
        _entities.erase(entity.entityID);
    }
    _isChanged = true;
}

void AverageHeightAnalyzerSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
	for (auto &entity : stream)
	{
		_entities.erase(entity);
	}
	_isChanged = true;
}

void AverageHeightAnalyzerSystem::Update(Environment &env, MessageBuilder &messageBuilder)
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

	if (_entityID.IsValid() == false)
	{
		_entityID = env.idGenerator.Generate();
		auto &c = messageBuilder.EntityAdded(_entityID);
		c.AddComponent(h);
	}
	else
	{
		messageBuilder.ComponentChanged(_entityID, h);
	}

	_isChanged = false;
}

INDIRECT_SYSTEM(CooldownUpdater)
{
    INDIRECT_ACCEPT_COMPONENTS(const Array<NegativeHeightCooldown> &);

private:
    std::map<EntityID, NegativeHeightCooldown> _cooldowns{};
};

void CooldownUpdater::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entity : stream)
    {
        _cooldowns[entity.entityID] = *entity.FindComponent<NegativeHeightCooldown>();
    }
}

void CooldownUpdater::ProcessMessages(const MessageStreamComponentAdded &stream)
{
    for (auto &entity : stream)
    {
        _cooldowns[entity.entityID] = *(NegativeHeightCooldown *)entity.component.data;
    }
}

void CooldownUpdater::ProcessMessages(const MessageStreamComponentChanged &stream)
{
    for (auto &entity : stream)
    {
        _cooldowns[entity.entityID] = *(NegativeHeightCooldown *)entity.component.data;
    }
}

void CooldownUpdater::ProcessMessages(const MessageStreamComponentRemoved &stream)
{
    for (auto &entity : stream)
    {
        _cooldowns.erase(entity.entityID);
    }
}

void CooldownUpdater::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entity : stream)
    {
        _cooldowns.erase(entity);
    }
}

void CooldownUpdater::Update(Environment &env, MessageBuilder &messageBuilder)
{
    for (auto it = _cooldowns.begin(); it != _cooldowns.end(); )
    {
        auto &[id, component] = *it;

        component.cooldown -= env.timeSinceLastFrame;
        if (component.cooldown <= 0)
        {
            messageBuilder.ComponentRemoved(id, component);
            it = _cooldowns.erase(it);
        }
        else
        {
            messageBuilder.ComponentChanged(id, component);
            ++it;
        }
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
    for (uiw index = 0; index < EntitiesToTest; ++index)
    {
        TestEntities::PreStreamedEntity entity;

		/*Transform t;
		t.position.x = rand() / (f32)RAND_MAX * 100 - 50;
		t.position.y = rand() / (f32)RAND_MAX * 100 - 50;
		t.position.z = rand() / (f32)RAND_MAX * 100 - 50;
		StreamComponent(t, entity);*/

		string name = "Entity"s + std::to_string(index);
		Name n;
		strcpy_s(n.name.data(), n.name.size(), name.c_str());
		StreamComponent(n, entity);

        stream.AddEntity(idGenerator.Generate(), move(entity));
    }
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

int main()
{
    StdLib::Initialization::Initialize({});

    constexpr bool isMT = false;

    auto stream = make_unique<TestEntities>();
    auto manager = SystemsManager::New(isMT);
    EntityIDGenerator idGenerator;

    GenerateScene(idGenerator, *manager, *stream);

    auto testPipelineGroup0 = manager->CreatePipelineGroup(5_ms, false);
    auto testPipelineGroup1 = manager->CreatePipelineGroup(6.5_ms, false);

    manager->Register(make_unique<TransformGeneratorSystem>(), testPipelineGroup0);
    manager->Register(make_unique<TransformHeightFixerSystem>(), testPipelineGroup0);
    manager->Register(make_unique<TransformFallingSystem>(), testPipelineGroup0);
	manager->Register(make_unique<AverageHeightAnalyzerSystem>(), testPipelineGroup1);
    manager->Register(make_unique<CooldownUpdater>(), testPipelineGroup1);

    vector<WorkerThread> workers;
    if (isMT)
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