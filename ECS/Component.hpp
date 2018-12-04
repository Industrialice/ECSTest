#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Component
    {
    };

    template <typename T> class _BaseComponent : public Component, public TypeIdentifiable<T>
    {
        //static_assert(std::is_pod_v<T>, "Component must be POD");

    public:
        using TypeIdentifiable<T>::GetTypeId;

		[[nodiscard]] pair<const TypeId *, uiw> Excludes() const
        {
            static constexpr TypeId excludes[] = {GetTypeId()};
            return {excludes, CountOf(excludes)};
        }
    };

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};
}