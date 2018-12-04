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
#include "ComponentProgrammer.hpp"
#include "ComponentSpouse.hpp"

using namespace ECSTest;

class GameOfLifeEntities : public EntitiesStream
{
    vector<StreamedEntity> _entities{};
    uiw _currentEntity{};

public:
    [[nodiscard]] virtual optional<StreamedEntity> Next() override
    {
        if (_currentEntity < _entities.size())
        {
            uiw index = _currentEntity++;
            return _entities[index];
        }
        return {};
    }

    void AddEntity(StreamedEntity &&entity)
    {
        _entities.emplace_back(move(entity));
    }
};

template <typename... Args> class ComponentsAllocator
{
	std::tuple<vector<Args>...> storages{};

public:
	template <typename T> uiw Allocate(const T &element = T())
	{
		auto &storage = std::get<vector<T>>(storages);
		uiw index = storage.size();
		storage.emplace_back(element);
		return index;
	}
};

static void GenerateScene(SystemsManager &manager)
{
    Funcs::Reinitialize(manager);

    ui32 entityId = 0;

    ComponentsAllocator<ComponentArtist, ComponentCompany, ComponentDateOfBirth,
        ComponentDesign, ComponentEmployee, ComponentFirstName, ComponentGender, 
        ComponentLastName, ComponentProgrammer, ComponentSpouse>
        allocator;

    struct Microsoft
    {
        EntityID id;
        ComponentCompany company;
        ComponentProgrammer programmer;
        ComponentArtist artist;
    } microsoft;
    microsoft.id = entityId++;
    microsoft.company.name = {"Microsoft"};
    microsoft.programmer.language = ComponentProgrammer::Language::C + ComponentProgrammer::Language::CPP + ComponentProgrammer::Language::CS;
    microsoft.programmer.skillLevel = ComponentProgrammer::SkillLevel::Junior;
    microsoft.artist.area = ComponentArtist::Area::TwoD + ComponentArtist::Area::Concept;
}

int main()
{
    StdLib::Initialization::Initialize({});

	GameOfLifeEntities stream;

	SystemsManager manager;
	manager.Start({}, ToArray(stream));

    printf("done\n");
    getchar();
}