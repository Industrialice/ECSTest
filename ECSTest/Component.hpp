#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Component
    {
        friend class SystemsManager;
		friend class ComponentChanger;

        class Entity *_entity = nullptr;
		bool _isEnabled = true;

    public:
		[[nodiscard]] class Entity &Entity();
		[[nodiscard]] const class Entity &Entity() const;
		[[nodiscard]] virtual TypeId Type() const = 0;
		[[nodiscard]] virtual pair<const TypeId *, uiw> Excludes() const = 0;
    };

    template <typename T> class _BaseComponent : public Component, public TypeIdentifiable<T>
    {
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

		[[nodiscard]] static constexpr bool IsExclusive()
        { 
            return true; 
        }
    };

	struct _ArrayOfComponentsBase
	{};

	template <typename T> class ArrayOfComponents : public _ArrayOfComponentsBase
	{
		T *const _components{};
		const uiw _count = 0;

	public:
		using ComponentType = T;

		ArrayOfComponents(T *components, uiw count) : _components(components), _count(count)
		{}

		[[nodiscard]] pair<T *, uiw> GetComponents()
		{
			return {_components, _count};
		}

		[[nodiscard]] pair<const T *, uiw> GetComponents() const
		{
			return {_components, _count};
		}

		[[nodiscard]] uiw Count() const
		{
			return _count;
		}
	};

	struct _SubtractiveComponentBase
	{};

	template <typename T> struct SubtractiveComponent : _SubtractiveComponentBase
	{
		using ComponentType = T;
	};
}