#pragma once

#include "ComponentProgrammer.hpp"

namespace ECSTest
{
    struct SystemTest : IndirectSystem<SystemTest>
    {
		using IndirectSystem::ProcessMessages;

		void Accept(const NonUnique<ComponentProgrammer> &);

    private:
        struct ProgrammerEntry
        {
            EntityID parent;
            ComponentProgrammer component;
        };

		virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override;
		virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override;
		virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override;
		virtual void Update(Environment &env) override;

        std::map<ComponentID, ProgrammerEntry> _programmers;
    };
}