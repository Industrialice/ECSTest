#pragma once

#include "Component.hpp"

namespace ECSTest
{
    class System
    {
    public:
        enum ComponentAvailability
        {
            Required,
            Optional,
			Subtractive
        };

        struct RequestedComponent
        {
            TypeId type;
            bool isWriteAccess;
			ComponentAvailability isRequired;
        };

		[[nodiscard]] virtual pair<const RequestedComponent *, uiw> RequestedComponents() const = 0;
		[[nodiscard]] virtual TypeId Type() const = 0;
		[[nodiscard]] virtual struct IndirectSystem *AsIndirectSystem();
		[[nodiscard]] virtual const struct IndirectSystem *AsIndirectSystem() const;
		[[nodiscard]] virtual struct DirectSystem *AsDirectSystem();
		[[nodiscard]] virtual const struct DirectSystem *AsDirectSystem() const;
        virtual void Accept(void **array) = 0;
	};

	struct IndirectSystem : public System
	{
		[[nodiscard]] virtual struct IndirectSystem *AsIndirectSystem() override final;
		[[nodiscard]] virtual const struct IndirectSystem *AsIndirectSystem() const override final;
        virtual void OnChanged(const shared_ptr<vector<Component>> &components) = 0;
        virtual void Commit() = 0;
        virtual void RemoveAll() = 0;
        virtual void Update() = 0;
	};

	struct DirectSystem : public System
	{
		[[nodiscard]] virtual struct DirectSystem *AsDirectSystem() override final;
		[[nodiscard]] virtual const struct DirectSystem *AsDirectSystem() const override final;
	};

	template <typename T> class _SystemTypeIdentifiable : public System, public TypeIdentifiable<T>
	{
	public:
		using TypeIdentifiable<T>::GetTypeId;

		[[nodiscard]] virtual TypeId Type() const override final
		{
			return GetTypeId();
		}
	};

	template <typename T> struct _IndirectSystem : public IndirectSystem, public _SystemTypeIdentifiable<T>
	{
	};

	template <typename T> struct _DirectSystem : public DirectSystem, public _SystemTypeIdentifiable<T>
	{
	};
}

#include "SystemHelpers.hpp"