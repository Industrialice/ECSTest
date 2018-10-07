#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Component
    {
        friend class SystemsManager;

        class Entity *_entity = nullptr;

    public:
        class Entity &Entity();
        const class Entity &Entity() const;
        virtual TypeId Type() const = 0;
        virtual pair<const TypeId *, uiw> Excludes() const = 0;
    };

    template <typename T> class _BaseComponent : public Component, public TypeIdentifiable<T>
    {
    public:
        using TypeIdentifiable<T>::GetTypeId;

        virtual TypeId Type() const override final
        {
            return GetTypeId();
        }

        virtual pair<const TypeId *, uiw> Excludes() const override
        {
            static constexpr TypeId excludes[] = {GetTypeId()};
            return {excludes, CountOf(excludes)};
        }

        static constexpr bool IsExclusive() 
        { 
            return true; 
        }
    };
}