#include "PreHeader.hpp"
#include "SystemTest.hpp"

using namespace ECSTest;

void SystemTest::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &item : stream)
    {
        printf("SystemTest received %u in MessageStreamEntityAdded\n", item.entityID.Hash());
    }
}

void SystemTest::ProcessMessages(const MessageStreamComponentChanged &stream)
{
    for (auto &item : stream)
    {
        printf("SystemTest received %u:%u:%u in MessageStreamComponentChanged\n", item.entityID.Hash(), (ui32)item.component.type.Hash(), item.component.id);
    }
}

void SystemTest::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
    for (auto &item : stream)
    {
        printf("SystemTest received %u in MessageStreamEntityRemoved\n", item.Hash());
    }
}

void SystemTest::Update(Environment &env, MessageBuilder &messageBuilder)
{
    auto threadId = std::this_thread::get_id();

    //printf("updating SystemTest on thread %u\n", *(ui32 *)&threadId);

    /*auto id = env.idGenerator.Generate();
    auto &builder = messageBuilder.EntityAdded(id);

    ComponentArtist artist;
    artist.area = ComponentArtist::Areas::Concept;
    builder.AddComponent(artist);

    Archetype archetype;
    archetype.Add(ComponentArtist::GetTypeId());

    messageBuilder.EntityRemoved(archetype, id);*/
}