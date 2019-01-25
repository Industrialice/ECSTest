#include "PreHeader.hpp"
#include "SystemGameInfo.hpp"

using namespace ECSTest;

void SystemGameInfo::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &item : stream)
    {
        printf("received %u\n", item.entityID.Hash());
    }
}

void SystemGameInfo::ProcessMessages(const MessageStreamEntityRemoved &stream)
{
}

void SystemGameInfo::Update(MessageBuilder &messageBuilder)
{
}