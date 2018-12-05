#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>

#include "ComponentArtist.hpp"
#include "ComponentCompany.hpp"
#include "ComponentDateOfBirth.hpp"
#include "ComponentDesign.hpp"
#include "ComponentEmployee.hpp"
#include "ComponentFirstName.hpp"
#include "ComponentGender.hpp"
#include "ComponentLastName.hpp"
#include "ComponentParents.hpp"
#include "ComponentProgrammer.hpp"
#include "ComponentSpouse.hpp"

using namespace ECSTest;

class GameOfLifeEntities : public EntitiesStream
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

    void AddEntity(PreStreamedEntity &&entity)
    {
        _entities.emplace_back(move(entity));
    }
};

template <typename T> void StreamComponent(const T &component, GameOfLifeEntities::PreStreamedEntity &preStreamed)
{
	EntitiesStream::ComponentDesc desc;
	auto componentData = make_unique<ui8[]>(sizeof(T));
	memcpy(componentData.get(), &component, sizeof(T));
	desc.alignmentOf = alignof(T);
	desc.excludes = T::Excludes();
	desc.sizeOf = sizeof(T);
	desc.type = T::GetTypeId();
	desc.data = componentData.get();
	preStreamed.componentsData.emplace_back(move(componentData));
	preStreamed.descs.emplace_back(desc);
}

template <typename... Args> GameOfLifeEntities::PreStreamedEntity Stream(EntityID entityId, const Args &... args)
{
	GameOfLifeEntities::PreStreamedEntity preStreamed;
	(StreamComponent(args, preStreamed), ...);
	preStreamed.streamed.entityId = entityId;
	preStreamed.streamed.components = ToArray(preStreamed.descs);
	return preStreamed;
}

static void GenerateScene(SystemsManager &manager, GameOfLifeEntities &stream)
{
	Funcs::Reinitialize(manager);

	ui32 entityId = 0;

	struct Microsoft
	{
		EntityID entityId;
		ComponentCompany company;
		ComponentProgrammer programmer;
		ComponentArtist artist;
	} microsoft;
	microsoft.entityId = entityId++;
	microsoft.company.name = {"Microsoft"};
	microsoft.programmer.language = ComponentProgrammer::Language::C + ComponentProgrammer::Language::CPP + ComponentProgrammer::Language::CS;
	microsoft.programmer.skillLevel = ComponentProgrammer::SkillLevel::Junior;
	microsoft.artist.area = ComponentArtist::Area::TwoD + ComponentArtist::Area::Concept;

	stream.AddEntity(Stream(entityId, microsoft.company, microsoft.programmer, microsoft.artist));
}

int main()
{
    StdLib::Initialization::Initialize({});

	GameOfLifeEntities stream;
	SystemsManager manager;

	GenerateScene(manager, stream);

	manager.Start({}, ToArray(stream));

    printf("done\n");
    getchar();
}