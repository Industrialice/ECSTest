#pragma once

#include "TypeIdentifiable.hpp"

#ifdef DEBUG
    #define IDINPUT <ui64 stableId, ui64 encoded0, ui64 encoded1, ui64 encoded2>
    #define IDOUTPUT <stableId, encoded0, encoded1, encoded2>
#else
    #define IDINPUT <ui64 stableId>
    #define IDOUTPUT <stableId>
#endif

namespace ECSTest
{
    class Component
    {
    };

    template IDINPUT class EMPTY_BASES _BaseComponent : public Component, public StableTypeIdentifiable IDOUTPUT
    {
    public:
        using StableTypeIdentifiable IDOUTPUT ::GetTypeId;

		[[nodiscard]] static constexpr bool IsUnique()
        {
            return true;
        }
    };

#ifdef DEBUG
    #define COMPONENT(name) struct name final : public _BaseComponent<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 0), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 1), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 2)>
#else
    #define COMPONENT(name) struct name final : public _BaseComponent<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>
#endif

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};
}

#undef IDINPUT
#undef IDOUTPUT