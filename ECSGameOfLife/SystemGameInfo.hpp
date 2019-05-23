#pragma once

#include <System.hpp>

#include "ComponentProgrammer.hpp"

namespace ECSTest
{
    struct SystemGameInfo : IndirectSystem<SystemGameInfo>
    {
		void Accept(NonUnique<ComponentProgrammer> *);

		using IndirectSystem::ProcessMessages;

    private:
        struct ProgrammerEntry
        {
            EntityID parent{};
            ComponentID componentId{};
            ComponentProgrammer::SkillLevel skillLevel{};

            bool operator < (const ProgrammerEntry &other) const
            {
                return std::tie(parent, componentId, skillLevel) < std::tie(other.parent, other.componentId, other.skillLevel);
            }
        };

		virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override;
		virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override;
		virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override;
		virtual void Update(Environment &env) override;

        std::set<ProgrammerEntry> _programmerEntities{};
    };
}