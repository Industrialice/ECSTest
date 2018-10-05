#pragma once

#include "Component.hpp"

namespace ECSTest
{
    class System
    {
    public:
        struct RequestedComponent
        {
            TypeId type;
            bool isReadAccess;
            bool isWriteAccess;
        };

        virtual pair<const RequestedComponent *, uiw> RequestedComponentsAll() const = 0;
		virtual pair<const RequestedComponent *, uiw> RequestedComponentsAny() const;
		virtual pair<const RequestedComponent *, uiw> RequestedComponentsOptional() const;
		virtual bool IsFatSystem() const;
		virtual TypeId Type() const = 0;
    };

	template <typename T> class _SystemTypeIdentifiable : public System, public TypeIdentifiable<T>
	{
	public:
		using TypeIdentifiable<T>::GetTypeId;

		virtual TypeId Type() const override final
		{
			return GetTypeId();
		}
	};
}