#pragma once

#include <TypeIdentifiable.hpp>

#ifdef DEBUG
    #define IDINPUT <bool isUnique, ui64 stableId, ui64 encoded0, ui64 encoded1, ui64 encoded2>
    #define IDOUTPUT <stableId, encoded0, encoded1, encoded2>
#else
    #define IDINPUT <bool isUnique, ui64 stableId>
    #define IDOUTPUT <stableId>
#endif

namespace ECSTest
{
    enum RequirementForComponent
    {
        Required,
        Optional,
        Subtractive
    };

    class Component
    {
    };

    template IDINPUT class EMPTY_BASES _BaseComponent : public Component, public StableTypeIdentifiable IDOUTPUT
    {
    public:
        using StableTypeIdentifiable IDOUTPUT ::GetTypeId;

		[[nodiscard]] static constexpr bool IsUnique()
        {
            return isUnique;
        }
    };

#ifdef DEBUG
    #define _CREATE_COMPONENT(name, isUnique) struct name final : public _BaseComponent<isUnique, Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 0), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 1), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), CompileTimeStrings::CharsPerNumber * 2)>
#else
    #define _CREATE_COMPONENT(name, isUnique) struct name final : public _BaseComponent<isUnique, Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>
#endif

    // TODO: replace it with a single COMPONENT macro that optionally accepts uniqueness
    #define COMPONENT(name) _CREATE_COMPONENT(name, true)
    #define NONUNIQUE_COMPONENT(name) _CREATE_COMPONENT(name, false)

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};
}

#undef IDINPUT
#undef IDOUTPUT