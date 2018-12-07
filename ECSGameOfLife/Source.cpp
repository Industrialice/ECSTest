#include "PreHeader.hpp"
#include <EntitiesStream.hpp>
#include <SystemsManager.hpp>
#include <stdio.h>
#include <tuple>

#include "ComponentArtist.hpp"
#include "ComponentCompany.hpp"
#include "ComponentDateOfBirth.hpp"
#include "ComponentDesigner.hpp"
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

template <ui32 N, typename... CompanyComponents> bool IsMatching(const std::variant<ComponentArtist, ComponentDesigner, ComponentProgrammer> &profession, const CompanyComponents &... companyComponents)
{
    auto tuple = std::make_tuple(companyComponents...);
    if constexpr (N < std::tuple_size_v<decltype(tuple)>)
    {
        auto companyComponent = std::get<N>(tuple);
        using componentType = decltype(companyComponent);
        if constexpr (std::is_same_v<componentType, ComponentProgrammer> || std::is_same_v<componentType, ComponentDesigner> || std::is_same_v<componentType, ComponentArtist>)
        {
            auto employeeComponent = std::get_if<componentType>(&profession);
            if (employeeComponent)
            {
                if constexpr (std::is_same_v<componentType, ComponentProgrammer>)
                {
                    if (companyComponent.language.Contains(employeeComponent->language))
                    {
                        if (companyComponent.skillLevel <= employeeComponent->skillLevel)
                        {
                            return true;
                        }
                    }
                }
                else if constexpr (std::is_same_v<componentType, ComponentDesigner>)
                {
                    if (companyComponent.area.Contains(employeeComponent->area))
                    {
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<componentType, ComponentArtist>)
                {
                    if (companyComponent.area.Contains(employeeComponent->area))
                    {
                        return true;
                    }
                }
            }
        }
        return IsMatching<N + 1>(profession, companyComponents...);
    }
    return false;
}

template <typename... CompanyComponents> void GenerateEmployees(GameOfLifeEntities &stream, ui32 &currentId, EntityID companyId, const CompanyComponents &... companyComponents)
{
    auto tuple = std::make_tuple(companyComponents...);
    ui32 generatedEmployees = 0;

    for (ui32 employeeIndex = 0; employeeIndex < 1000; ++employeeIndex)
    {
        std::variant<ComponentArtist, ComponentDesigner, ComponentProgrammer> profession;
        int choice = rand() % std::variant_size_v<decltype(profession)>;
        if (choice == 0)
        {
            ComponentArtist artist;
            ui32 areasCount = 1;
            int areasCountRand = rand() % 10;
            if (areasCountRand > 4) ++areasCount;
            if (areasCountRand > 8) ++areasCount;
            for (ui32 areaIndex = 0; areaIndex < areasCount; ++areaIndex)
            {
                int newArea = rand() % 3;
                if (newArea == 0) artist.area += ComponentArtist::Area::Concept;
                if (newArea == 1) artist.area += ComponentArtist::Area::ThreeD;
                if (newArea == 2) artist.area += ComponentArtist::Area::TwoD;
            }
            profession = artist;
        }
        if (choice == 1)
        {
            ComponentDesigner design;
            ui32 areasCount = 1;
            int areasCountRand = rand() % 5;
            if (areasCountRand > 3) ++areasCount;
            for (ui32 areaIndex = 0; areaIndex < areasCount; ++areaIndex)
            {
                int newArea = rand() % 2;
                if (newArea == 0) design.area += ComponentDesigner::Area::UXUI;
                if (newArea == 1) design.area += ComponentDesigner::Area::Level;
            }
            profession = design;
        }
        if (choice == 2)
        {
            ComponentProgrammer programmer;
            programmer.skillLevel = ComponentProgrammer::SkillLevel::Junior;
            ui32 areasCount = 1;
            int areasCountRand = rand() % 10;
            if (areasCountRand > 2) ++areasCount;
            if (areasCountRand > 6) ++areasCount;
            if (areasCountRand > 8) ++areasCount;
            for (ui32 areaIndex = 0; areaIndex < areasCount; ++areaIndex)
            {
                int newArea = rand() % 7;
                if (newArea == 0) programmer.language += ComponentProgrammer::Language::C;
                if (newArea == 1) programmer.language += ComponentProgrammer::Language::CPP;
                if (newArea == 2) programmer.language += ComponentProgrammer::Language::CS;
                if (newArea == 3) programmer.language += ComponentProgrammer::Language::Java;
                if (newArea == 4) programmer.language += ComponentProgrammer::Language::JS;
                if (newArea == 5) programmer.language += ComponentProgrammer::Language::PHP;
                if (newArea == 6) programmer.language += ComponentProgrammer::Language::Python;
            }
            int skillLevelRand = rand() % 50;
            if (skillLevelRand > 35) programmer.skillLevel = ComponentProgrammer::SkillLevel::Middle;
            if (skillLevelRand > 45) programmer.skillLevel = ComponentProgrammer::SkillLevel::Senior;
            profession = programmer;
        }
        if (IsMatching<0>(profession, companyComponents...))
        {
            static constexpr const char *firstNamesMale[] = {"Dave", "Bill", "Donald", "Vladimir", "Bashar", "Hussein", "Ossama", "Doctor", "Man", "Barack", "Guy", "Martin", "Max"};
            static constexpr const char *firstNamesFemale[] = {"Mria", "Colgate", "Alyx", "Hillary", "Michelle"};
            static constexpr const char *lastNames[] = {"Clinton", "Merkel", "Trump", "Putin", "Assad", "Laden", "Obama", "Payne"};

            GameOfLifeEntities::PreStreamedEntity preStreamed;
            std::visit([&preStreamed](const auto &arg) { StreamComponent(arg, preStreamed); }, profession);
            ComponentGender gender;
            gender.isMale = rand() % 2 == 0;
            StreamComponent(gender, preStreamed);
            ComponentDateOfBirth dateOfBirth;
            dateOfBirth.dateOfBirth = (rand() % (30 * 365)) + (20 * 365);
            StreamComponent(dateOfBirth, preStreamed);
            ComponentEmployee employee;
            employee.employer = companyId;
            StreamComponent(employee, preStreamed);
            ComponentFirstName firstName;
            if (gender.isMale)
            {
                auto name = firstNamesMale[rand() % CountOf(firstNamesMale)];
                strcpy(firstName.name.data(), name);
            }
            else
            {
                auto name = firstNamesFemale[rand() % CountOf(firstNamesFemale)];
                strcpy(firstName.name.data(), name);
            }
            StreamComponent(firstName, preStreamed);
            ComponentLastName lastName;
            strcpy(lastName.name.data(), lastNames[rand() % CountOf(lastNames)]);
            StreamComponent(lastName, preStreamed);
            EntityID employeeId = currentId++;
            
            preStreamed.streamed.entityId = employeeId;
            preStreamed.streamed.components = ToArray(preStreamed.descs);

            stream.AddEntity(move(preStreamed));

            ++generatedEmployees;
        }
    }

    printf("Generated %u employees for %s\n", generatedEmployees, std::get<ComponentCompany>(tuple).name.data());
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
	stream.AddEntity(Stream(microsoft.entityId, microsoft.company, microsoft.programmer, microsoft.artist));
    GenerateEmployees(stream, entityId, microsoft.entityId, microsoft.company, microsoft.programmer, microsoft.artist);

    struct EA
    {
        EntityID entityId;
        ComponentCompany company;
        ComponentProgrammer programmer;
        ComponentArtist artist;
        ComponentDesigner designer;
    } ea;
    ea.entityId = entityId++;
    ea.company.name = {"EA"};
    ea.programmer.language = ComponentProgrammer::Language::CPP;
    ea.programmer.skillLevel = ComponentProgrammer::SkillLevel::Junior;
    ea.artist.area = ComponentArtist::Area::Concept + ComponentArtist::Area::ThreeD + ComponentArtist::Area::TwoD;
    ea.designer.area = ComponentDesigner::Area::Level + ComponentDesigner::Area::UXUI;
    stream.AddEntity(Stream(ea.entityId, ea.company, ea.programmer, ea.artist, ea.designer));
    GenerateEmployees(stream, entityId, ea.entityId, ea.company, ea.programmer, ea.artist, ea.designer);
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