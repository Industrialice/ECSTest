#include "PreHeader.hpp"
#include "SystemTest.hpp"

using namespace ECSTest;

void SystemTest::ProcessMessages(const MessageStreamEntityAdded &stream)
{
    for (auto &item : stream)
    {
        //printf("SystemTest received %u in MessageStreamEntityAdded\n", item.entityID.Hash());

        for (auto &component : item.components)
        {
            if (component.type == ComponentProgrammer::GetTypeId())
            {
                auto reinterpreted = (ComponentProgrammer *)component.data;
                ProgrammerEntry entry;
                entry.parent = item.entityID;
                entry.component = *reinterpreted;
                auto [it, result] = _programmers.insert({component.id, entry});
                ASSUME(result);
            }
        }
    }
}

void SystemTest::ProcessMessages(const MessageStreamComponentChanged &stream)
{
    for (auto &item : stream)
    {
        //printf("SystemTest received %u:%u:%u in MessageStreamComponentChanged\n", item.entityID.Hash(), (ui32)item.component.type.Hash(), item.component.id);
    }

    if (stream.Type() == ComponentProgrammer::GetTypeId())
    {
        for (auto &item : stream)
        {
            auto rei = *(ComponentProgrammer *)item.component.data;
            auto search = _programmers.find(item.component.id);
            ASSUME(search != _programmers.end());
            printf("SystemTest receieved change from %u to %u for component %u\n", (ui32)search->second.component.language, (ui32)rei.language, item.component.id);
            search->second.component = rei;
        }
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


}