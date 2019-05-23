#include "PreHeader.hpp"
#include "SystemTest.hpp"

using namespace ECSTest;

void SystemTest::ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream)
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

void SystemTest::ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream)
{
    for (const auto &item : stream.Enumerate<ComponentProgrammer>())
    {
        auto rei = item.component;
        auto search = _programmers.find(item.componentID);
        ASSUME(search != _programmers.end());
        printf("SystemTest receieved change from %u to %u for component %u\n", (ui32)search->second.component.language, (ui32)rei.language, item.componentID.ID());
        search->second.component = rei;
    }
}

void SystemTest::ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream)
{
    for (auto &item : stream)
    {
        printf("SystemTest received %u in MessageStreamEntityRemoved\n", item.Hash());
    }
}

void SystemTest::Update(Environment &env)
{
    auto threadId = std::this_thread::get_id();

    //printf("updating SystemTest on thread %u\n", *(ui32 *)&threadId);


}