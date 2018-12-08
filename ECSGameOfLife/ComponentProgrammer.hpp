#pragma once

#include <Component.hpp>

namespace ECSTest
{
	COMPONENT(ComponentProgrammer)
    {
        struct Languages
        {
            static constexpr struct Language : EnumCombinable<Language, ui32, true>
            {} Undefined = Language::Create(0),
                CPP = Language::Create(1 << 0),
                CS = Language::Create(1 << 1),
                C = Language::Create(1 << 2),
                PHP = Language::Create(1 << 3),
                JS = Language::Create(1 << 4),
                Java = Language::Create(1 << 5),
                Python = Language::Create(1 << 6);
        };

        Languages::Language language;
        enum class SkillLevel { Junior, Middle, Senior } skillLevel;
    };
}