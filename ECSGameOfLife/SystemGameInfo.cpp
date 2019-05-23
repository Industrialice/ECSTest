#include "PreHeader.hpp"
#include "SystemGameInfo.hpp"

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

void SystemGameInfo::ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream)
{
    for (auto &item : stream)
    {
        printf("SystemGameInfo received %u in MessageStreamEntityAdded\n", item.entityID.Hash());

        for (auto &component : item.components)
        {
            if (component.type == ComponentProgrammer::GetTypeId())
            {
                ProgrammerEntry entry;
                entry.parent = item.entityID;
                entry.componentId = component.id;
                entry.skillLevel = ((ComponentProgrammer *)component.data)->skillLevel;
                auto [it, result] = _programmerEntities.insert(entry);
                ASSUME(result);
            }
        }
    }
}

void SystemGameInfo::ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream)
{
    //for (auto &item : stream)
    //{
    //    printf("SystemGameInfo received %u:%u:%u in MessageStreamComponentChanged\n", item.entityID.Hash(), (ui32)item.component.type.Hash(), item.component.id.ID());
    //}
}

void SystemGameInfo::ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream)
{
    for (auto &item : stream)
    {
        printf("SystemGameInfo received %u in MessageStreamEntityRemoved\n", item.Hash());
    }
}

void SystemGameInfo::Update(Environment &env)
{
    auto threadId = std::this_thread::get_id();

    //printf("updating SystemGameInfo on thread %u\n", *(ui32 *)&threadId);

    auto id = env.entityIdGenerator.Generate();
    auto &builder = env.messageBuilder.AddEntity(id);

    ComponentArtist artist;
    artist.area = ComponentArtist::Areas::Concept;
    builder.AddComponent(artist);

    TypeId types[] = {ComponentArtist::GetTypeId()};
    Archetype archetype = Archetype::Create<TypeId>(ToArray(types));

    env.messageBuilder.RemoveEntity(id, archetype);

    for (uiw index = 0; index < 10; ++index)
    {
        if (_programmerEntities.empty())
        {
            break;
        }

        ProgrammerEntry entry = *_programmerEntities.begin();
        _programmerEntities.erase(_programmerEntities.begin());
        
        ComponentProgrammer changed;
        changed.language = ComponentProgrammer::Languages::Java;
        changed.skillLevel = entry.skillLevel;

        env.messageBuilder.ComponentChanged(entry.parent, changed, entry.componentId);
    }
}