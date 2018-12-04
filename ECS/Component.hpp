#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Component
    {
        friend class SystemsManager;
		friend class ComponentChanger;

		bool _isEnabled = true;

    public:
		[[nodiscard]] class Entity &Entity();
		[[nodiscard]] const class Entity &Entity() const;
		[[nodiscard]] virtual TypeId Type() const = 0;
		[[nodiscard]] virtual pair<const TypeId *, uiw> Excludes() const = 0;
		[[nodiscard]] virtual pair<uiw, uiw> SizeAlignment() const = 0;
    };

    template <typename T> class _BaseComponent : public Component, public TypeIdentifiable<T>
    {
        //static_assert(std::is_pod_v<T>, "Component must be POD");

    public:
        using TypeIdentifiable<T>::GetTypeId;

		[[nodiscard]] virtual TypeId Type() const override final
        {
            return GetTypeId();
        }

		[[nodiscard]] virtual pair<const TypeId *, uiw> Excludes() const override
        {
            static constexpr TypeId excludes[] = {GetTypeId()};
            return {excludes, CountOf(excludes)};
        }

		[[nodiscard]] virtual pair<uiw, uiw> SizeAlignment() const override
		{
			return {sizeof(T), alignof(T)};
		}
    };

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};
}