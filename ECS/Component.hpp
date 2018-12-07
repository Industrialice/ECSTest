#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Component
    {
    };

    template 
    #ifdef DEBUG
        <ui64 stableId, ui64 encoded0, ui64 encoded1, ui64 encoded2>
    #else
        <ui64 stableId>
    #endif
        class EMPTY_BASES _BaseComponent : public Component, public StableTypeIdentifiable
    #ifdef DEBUG
        <stableId, encoded0, encoded1, encoded2>
    #else
        <stableId>
    #endif
    {
    public:
        using StableTypeIdentifiable
        #ifdef DEBUG
            <stableId, encoded0, encoded1, encoded2>
        #else
            <stableId>
        #endif
            ::GetTypeId;

		[[nodiscard]] static pair<const StableTypeId *, uiw> Excludes()
        {
            static constexpr StableTypeId excludes[] = {GetTypeId()};
            return {excludes, CountOf(excludes)};
        }
    };

#ifdef DEBUG
    #define COMPONENT(name) struct name : public _BaseComponent<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name)), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), 0), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), 7), \
        CompileTimeStrings::EncodeASCII(TOSTR(name), CountOf(TOSTR(name)), 14)>
#else
    #define COMPONENT(name) struct name : public _BaseComponent<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>
#endif

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};
}