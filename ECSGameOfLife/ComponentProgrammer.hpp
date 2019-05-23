#pragma once

#include <Component.hpp>

namespace ECSTest
{
	struct ComponentProgrammer : NonUniqueComponent<ComponentProgrammer>
    {
        enum class Languages : ui8
        {
            Undefined,
            CPP,
            CS,
            C,
            PHP,
            JS,
            Java,
            Python
        } language;

        enum class SkillLevel : ui8 
        { 
            Junior, Middle, Senior 
        } skillLevel;
    };
}