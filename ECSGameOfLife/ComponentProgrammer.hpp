#pragma once

#include <Component.hpp>

namespace ECSTest
{
    struct ComponentProgrammer : public _BaseComponent<ComponentProgrammer>
    {
		ENUM_COMBINABLE(Language, ui32,
            Undefined,
			CPP = Funcs::BitPos(0),
			CS = Funcs::BitPos(1),
			C = Funcs::BitPos(2),
			PHP = Funcs::BitPos(3),
			JS = Funcs::BitPos(4),
			Java = Funcs::BitPos(5),
			Python = Funcs::BitPos(6));

        Language language;
        enum class SkillLevel { Junior, Middle, Senior } skillLevel;
    };

	ENUM_COMBINABLE_OPS(ComponentProgrammer::Language, ui32);
}