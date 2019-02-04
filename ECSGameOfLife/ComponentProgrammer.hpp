#pragma once

#include <Component.hpp>

namespace ECSTest
{
	NONUNIQUE_COMPONENT(ComponentProgrammer)
    {
        enum class Languages
        {
            Undefined,
            CPP,
            CS,
            C,
            PHP,
            JS,
            Java,
            Python
        };

        Languages language;
        enum class SkillLevel { Junior, Middle, Senior } skillLevel;
    };
}