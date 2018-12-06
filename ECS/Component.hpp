#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Component
    {
    };

    template <ui64 stableId> class EMPTY_BASES _BaseComponent : public Component, public StableTypeIdentifiable<stableId>
    {
    public:
        using StableTypeIdentifiable<stableId>::GetTypeId;

		[[nodiscard]] static pair<const StableTypeId *, uiw> Excludes()
        {
            static constexpr StableTypeId excludes[] = {GetTypeId()};
            return {excludes, CountOf(excludes)};
        }
    };

    #define COMPONENT(name) struct name : public _BaseComponent<Hash::FNVHashCT<Hash::Precision::P64, char, CountOf(TOSTR(name)), true>(TOSTR(name))>

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};
}