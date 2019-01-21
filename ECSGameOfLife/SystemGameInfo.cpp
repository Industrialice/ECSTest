#include "PreHeader.hpp"
#include "SystemGameInfo.hpp"

using namespace ECSTest;

void SystemGameInfo::ProcessMessages(Archetype achetype, MessageStreamEntityRemoved &stream)
{
}

void SystemGameInfo::ProcessMessages(Archetype achetype, MessageStreamEntityAdded &stream)
{
	for (auto item = stream.Next(); item; item = stream.Next())
	{
		printf("received %u\n", item->entityID.Hash());
	}
}

void SystemGameInfo::Update(MessageBuilder &messageBuilder)
{
}