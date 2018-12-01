#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentProgrammer : public _BaseComponent<ComponentProgrammer>
    {
        ENUM_COMBINABLE(Language, ui32,
            CPP, CS, C, PHP, JS, Java, Python);
        Language language;
        enum class SkillLevel { Junior, Middle, Senior } skillLevel;
    };
}