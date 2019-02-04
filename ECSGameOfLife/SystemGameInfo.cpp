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

void SystemGameInfo::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &item : stream)
    {
        printf("SystemGameInfo received %u in MessageStreamEntityAdded\n", item.entityID.Hash());
    }
}

void SystemGameInfo::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &item : stream)
    {
        printf("SystemGameInfo received %u in MessageStreamEntityRemoved\n", item.Hash());
    }
}

void SystemGameInfo::Update(Environment &env, MessageBuilder &messageBuilder)
{
    auto threadId = std::this_thread::get_id();

    printf("updating SystemGameInfo on thread %u\n", *(ui32 *)&threadId);

    auto id = env.idGenerator.Generate();
    auto &builder = messageBuilder.EntityAdded(id);

    ComponentArtist artist;
    artist.area = ComponentArtist::Areas::Concept;
    builder.AddComponent(artist);

    StableTypeId types[] = {ComponentArtist::GetTypeId()};
    Archetype archetype = Archetype::Create<StableTypeId>(ToArray(types));

    messageBuilder.EntityRemoved(archetype, id);
}