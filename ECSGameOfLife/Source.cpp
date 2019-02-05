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

#include "SystemGameInfo.hpp"
#include "SystemTest.hpp"

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

    void AddEntity(EntityID id, PreStreamedEntity &&entity)
    {
        _entities.emplace_back(move(entity));
        _entities.back().streamed.components = ToArray(_entities.back().descs);
        _entities.back().streamed.entityId = id;
    }
};

template <typename T> void StreamComponent(const T &component, GameOfLifeEntities::PreStreamedEntity &preStreamed)
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

template <typename... Args> GameOfLifeEntities::PreStreamedEntity Stream(const Args &... args)
{
	GameOfLifeEntities::PreStreamedEntity preStreamed;
	(StreamComponent(args, preStreamed), ...);
	return preStreamed;
}

template <ui32 N = 0, typename... CompanyComponents> bool IsMatching(const std::variant<ComponentArtist, ComponentDesigner, ComponentProgrammer> &profession, const CompanyComponents &... companyComponents)
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
                    if (companyComponent.language == employeeComponent->language)
                    {
                        if (companyComponent.skillLevel <= employeeComponent->skillLevel)
                        {
                            return true;
                        }
                    }
                }
                else if constexpr (std::is_same_v<componentType, ComponentDesigner>)
                {
                    if (companyComponent.area == employeeComponent->area)
                    {
                        return true;
                    }
                }
                else if constexpr (std::is_same_v<componentType, ComponentArtist>)
                {
                    if (companyComponent.area == employeeComponent->area)
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

template <typename... CompanyComponents> void GenerateEmployees(GameOfLifeEntities &stream, EntityIDGenerator &idGenerator, EntityID companyId, const CompanyComponents &... companyComponents)
{
    auto tuple = std::make_tuple(companyComponents...);
    ui32 generatedEmployees = 0;

    for (ui32 employeeIndex = 0; employeeIndex < 1000; ++employeeIndex)
    {
        static vector<std::variant<ComponentArtist, ComponentDesigner, ComponentProgrammer>> choices;
        if (choices.empty())
        {
            using aa = ComponentArtist::Areas;
            ComponentArtist a;
            a.area = aa::Concept;
            choices.push_back(a);
            a.area = aa::ThreeD;
            choices.push_back(a);
            a.area = aa::TwoD;
            choices.push_back(a);

            using pl = ComponentProgrammer::Languages;
            ComponentProgrammer p;
            p.language = pl::C;
            choices.push_back(p);
            p.language = pl::CPP;
            choices.push_back(p);
            p.language = pl::CS;
            choices.push_back(p);
            p.language = pl::Java;
            choices.push_back(p);
            p.language = pl::JS;
            choices.push_back(p);
            p.language = pl::PHP;
            choices.push_back(p);
            p.language = pl::Python;
            choices.push_back(p);

            using da = ComponentDesigner::Areas;
            ComponentDesigner d;
            d.area = da::Level;
            choices.push_back(d);
            d.area = da::UXUI;
            choices.push_back(d);
        }

        vector<std::variant<ComponentArtist, ComponentDesigner, ComponentProgrammer>> professions;

        int choicesCount = rand() % 5 + 1;
        for (int c = 0; c < choicesCount; ++c)
        {
            auto choice = choices[rand() % choices.size()];
            bool isAdd = true;

            if (auto p = std::get_if<ComponentProgrammer>(&choice); p)
            {
                p->skillLevel = ComponentProgrammer::SkillLevel::Junior;
                int skillLevelRand = rand() % 50;
                if (skillLevelRand > 35) p->skillLevel = ComponentProgrammer::SkillLevel::Middle;
                if (skillLevelRand > 45) p->skillLevel = ComponentProgrammer::SkillLevel::Senior;

                for (const auto &profession : professions)
                {
                    if (auto existing = std::get_if<ComponentProgrammer>(&profession); existing)
                    {
                        if (p->language == existing->language)
                        {
                            isAdd = false;
                            break;
                        }
                    }
                }
            }
            else if (auto a = std::get_if<ComponentArtist>(&choice); a)
            {
                for (const auto &profession : professions)
                {
                    if (auto existing = std::get_if<ComponentArtist>(&profession); existing)
                    {
                        if (a->area == existing->area)
                        {
                            isAdd = false;
                            break;
                        }
                    }
                }
            }
            else
            {
                auto d = std::get_if<ComponentDesigner>(&choice);
                ASSUME(d);
                for (const auto &profession : professions)
                {
                    if (auto existing = std::get_if<ComponentDesigner>(&profession); existing)
                    {
                        if (d->area == existing->area)
                        {
                            isAdd = false;
                            break;
                        }
                    }
                }
            }

            if (isAdd)
            {
                professions.push_back(choice);
            }
        }

        bool isMatchingAny = false;
        for (const auto &profession : professions)
        {
            if (IsMatching(profession, companyComponents...))
            {
                isMatchingAny = true;
                break;
            }
        }

        if (isMatchingAny)
        {
            static constexpr const char *firstNamesMale[] = {"Dave", "Bill", "Donald", "Vladimir", "Bashar", "Hussein", "Ossama", "Doctor", "Man", "Barack", "Guy", "Martin", "Max"};
            static constexpr const char *firstNamesFemale[] = {"Mria", "Colgate", "Alyx", "Hillary", "Michelle"};
            static constexpr const char *lastNames[] = {"Clinton", "Merkel", "Trump", "Putin", "Assad", "Laden", "Obama", "Payne"};

            GameOfLifeEntities::PreStreamedEntity preStreamed;
            for (const auto &profession : professions)
            {
                std::visit([&preStreamed](const auto &arg) { StreamComponent(arg, preStreamed); }, profession);
            }
            ComponentGender gender;
            gender.isMale = rand() % 2 == 0;
            StreamComponent(gender, preStreamed);
            ComponentDateOfBirth dateOfBirth;
            dateOfBirth.dateOfBirth = (rand() % (30 * 365)) + (20 * 365); // 20..50 years
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
            EntityID employeeId = idGenerator.Generate();
            
            stream.AddEntity(employeeId, move(preStreamed));

            ++generatedEmployees;
        }
    }

    printf("Generated %u employees for %s\n", generatedEmployees, std::get<ComponentCompany>(tuple).name.data());
}

static void GenerateScene(EntityIDGenerator &idGenerator, SystemsManager &manager, GameOfLifeEntities &stream)
{
	struct Microsoft
	{
		EntityID entityId;
		ComponentCompany company;
		ComponentProgrammer programmerC, programmerCPP, programmerCS;
		ComponentArtist artistTwoD, artistConcept;
	} microsoft;
	microsoft.entityId = idGenerator.Generate();
	microsoft.company.name = {"Microsoft"};
    microsoft.programmerC.language = ComponentProgrammer::Languages::C;
    microsoft.programmerC.skillLevel = ComponentProgrammer::SkillLevel::Middle;
    microsoft.programmerCPP.language = ComponentProgrammer::Languages::CPP;
    microsoft.programmerCPP.skillLevel = ComponentProgrammer::SkillLevel::Junior;
    microsoft.programmerCS.language = ComponentProgrammer::Languages::CS;
	microsoft.programmerCS.skillLevel = ComponentProgrammer::SkillLevel::Junior;
	microsoft.artistTwoD.area = ComponentArtist::Areas::TwoD;
    microsoft.artistConcept.area = ComponentArtist::Areas::Concept;
	stream.AddEntity(microsoft.entityId, Stream(microsoft.company, microsoft.programmerC, microsoft.programmerCPP, microsoft.programmerCS, microsoft.artistTwoD, microsoft.artistConcept));
    GenerateEmployees(stream, idGenerator, microsoft.entityId, microsoft.company, microsoft.programmerC, microsoft.programmerCPP, microsoft.programmerCS, microsoft.artistTwoD, microsoft.artistConcept);

    struct EA
    {
        EntityID entityId;
        ComponentCompany company;
        ComponentProgrammer programmer;
        ComponentArtist artistConcept, artistTwoD, artistThreeD;
        ComponentDesigner designerLevel, designerUXUI;
    } ea;
    ea.entityId = idGenerator.Generate();
    ea.company.name = {"EA"};
    ea.programmer.language = ComponentProgrammer::Languages::CPP;
    ea.programmer.skillLevel = ComponentProgrammer::SkillLevel::Junior;
    ea.artistConcept.area = ComponentArtist::Areas::Concept;
    ea.artistTwoD.area = ComponentArtist::Areas::ThreeD;
    ea.artistThreeD.area = ComponentArtist::Areas::TwoD;
    ea.designerLevel.area = ComponentDesigner::Areas::Level;
    ea.designerUXUI.area = ComponentDesigner::Areas::UXUI;
    stream.AddEntity(ea.entityId, Stream(ea.company, ea.programmer, ea.artistConcept, ea.artistTwoD, ea.artistThreeD, ea.designerLevel, ea.designerUXUI));
    GenerateEmployees(stream, idGenerator, ea.entityId, ea.company, ea.programmer, ea.artistConcept, ea.artistTwoD, ea.artistThreeD, ea.designerLevel, ea.designerUXUI);
}

int main()
{
    StdLib::Initialization::Initialize({});

    auto stream = make_unique<GameOfLifeEntities>();
    auto manager = SystemsManager::New();;
    EntityIDGenerator idGenerator;

	GenerateScene(idGenerator, *manager, *stream);

	auto gameInfoPipelineGroup = manager->CreatePipelineGroup(1000'0000, true);
	manager->Register(make_unique<SystemGameInfo>(), gameInfoPipelineGroup);
    manager->Register(make_unique<SystemTest>(), gameInfoPipelineGroup);

	vector<WorkerThread> workers(SystemInfo::LogicalCPUCores());

    vector<unique_ptr<EntitiesStream>> streams;
    streams.push_back(move(stream));
    manager->Start(move(idGenerator), move(workers), move(streams));
    std::this_thread::sleep_for(2500ms);
    manager->Stop(true);

    system("pause");
}