#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>
#include <set>

using namespace ECSTest;

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
    INDIRECT_ACCEPT_COMPONENTS(Array<Transform> &);

private:
	std::map<EntityID, Transform> _entitiesToFix{};
    HeightFixerInfo _info{};
    EntityID _infoId{};
};

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &entity : stream)
    {
        auto pred = [](const SerializedComponent &c) { return c.type == Transform::GetTypeId(); };
        auto it = std::find_if(entity.components.begin(), entity.components.end(), pred);
        Transform t = *(Transform *)it->data;
        if (t.position.y < 0)
        {
            t.position.y = 100;
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
                t.position.y = 100;
                _entitiesToFix[entity.entityID] = t;
            }
        }
    }
}

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamComponentChanged &stream)
{
	for (auto &entity : stream)
	{
		ASSUME(entity.component.type == Transform::GetTypeId());
		Transform t = *(Transform *)entity.component.data;
		if (t.position.y < 0)
		{
			t.position.y = 100;
			_entitiesToFix[entity.entityID] = t;
		}
	}
}

void TransformHeightFixerSystem::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &entity : stream)
    {
        _entitiesToFix.erase(entity);
    }
}

void TransformHeightFixerSystem::Update(Environment &env, MessageBuilder &messageBuilder)
{
    ++_info.runTimes;
    _info.heightsFixed += (ui32)_entitiesToFix.size();

	for (auto &entity : _entitiesToFix)
	{
		messageBuilder.ComponentChanged(entity.first, entity.second);
	}
	_entitiesToFix.clear();

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
		auto pred = [](const SerializedComponent &c) { return c.type == Transform::GetTypeId(); };
		auto it = std::find_if(entity.components.begin(), entity.components.end(), pred);
		_entities[entity.entityID] = *(Transform *)it->data;
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
		component.position.y -= env.timeSinceLastFrame * 9.81f;
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
		auto pred = [](const SerializedComponent &c) { return c.type == Transform::GetTypeId(); };
		auto it = std::find_if(entity.components.begin(), entity.components.end(), pred);
		_entities[entity.entityID] = (*(Transform *)it->data).position.y;
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
    for (uiw index = 0; index < 100; ++index)
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
	while (auto entity = stream.Next())
	{
		for (auto &c : entity->components)
		{
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

	printf("finished %s\n", isFirstPass ? "first" : "second");
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

    std::this_thread::sleep_for(500ms);

    manager->Pause(true);

    ecsstream = manager->StreamOut();

    PrintStreamInfo(*ecsstream, false);

    manager->Stop(true);

    system("pause");
}