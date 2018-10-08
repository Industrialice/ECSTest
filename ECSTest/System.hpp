#pragma once

#include "Component.hpp"

namespace ECSTest
{
    class System
    {
    public:
        enum ComponentOptionality
        {
            Required,
            Any,
            Optional
        };

        struct RequestedComponent
        {
            TypeId type;
            bool isWriteAccess;
            bool isRequired;
        };

		[[nodiscard]] virtual pair<const RequestedComponent *, uiw> RequestedComponents() const = 0;
		[[nodiscard]] virtual bool IsFatSystem() const;
        virtual void AcceptComponents(void *first, ...) const; // used only for slim systems
		[[nodiscard]] virtual TypeId Type() const = 0;
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
}

#include "SystemHelpers.hpp"