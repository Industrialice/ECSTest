#pragma once

#include <System.hpp>

#include "ComponentProgrammer.hpp"

namespace ECSTest
{
    INDIRECT_SYSTEM(SystemTest)
    {
        INDIRECT_ACCEPT_COMPONENTS(const Array<ComponentProgrammer> &artist);

    private:
        struct ProgrammerEntry
        {
            EntityID parent;
            ComponentProgrammer component;
        };

        std::map<ComponentID, ProgrammerEntry> _programmers;
    };
}