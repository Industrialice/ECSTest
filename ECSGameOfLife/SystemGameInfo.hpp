#pragma once

#include <System.hpp>

#include "ComponentProgrammer.hpp"

namespace ECSTest
{
    INDIRECT_SYSTEM(SystemGameInfo)
    {
		INDIRECT_ACCEPT_COMPONENTS();

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

        std::set<ProgrammerEntry> _programmerEntities{};
    };
}