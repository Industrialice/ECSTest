#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>

#include "ComponentArtist.hpp"
#include "ComponentCompany.hpp"
#include "ComponentDateOfBirth.hpp"
#include "ComponentDesign.hpp"
#include "ComponentFirstName.hpp"
#include "ComponentGender.hpp"
#include "ComponentLastName.hpp"
#include "ComponentProgrammer.hpp"
#include "ComponentSpouse.hpp"

using namespace ECSTest;

class GameOfLifeEntities : public EntitiesStream
{
public:
    [[nodiscard]] virtual optional<StreamedEntity> Next() override
    {
        return {};
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

int main()
{
    StdLib::Initialization::Initialize({});

	ComponentsAllocator<ComponentArtist, ComponentCompany, ComponentDateOfBirth,
		ComponentDesign, ComponentFirstName, ComponentGender, ComponentLastName,
		ComponentProgrammer, ComponentSpouse> 
		allocator;

	GameOfLifeEntities stream;

	SystemsManager manager;
	manager.Start({}, ToArray(stream));

    printf("done\n");
    getchar();
}