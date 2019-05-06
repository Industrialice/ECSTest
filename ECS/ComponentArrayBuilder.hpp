#pragma once

#include "SerializedComponent.hpp"
#include "IEntitiesStream.hpp"

namespace ECSTest
{
    class ComponentArrayBuilder
    {
        friend class SystemsManagerMT;
        friend class SystemsManagerST;
        friend class MessageBuilder;

        vector<SerializedComponent> _components{};
        vector<ui8> _data{};

        void Clear();

    public:
        ComponentArrayBuilder() = default;
        ComponentArrayBuilder(ComponentArrayBuilder &&) = default;
        ComponentArrayBuilder &operator = (ComponentArrayBuilder &&) = default;

        ComponentArrayBuilder &AddComponent(const IEntitiesStream::ComponentDesc &desc, ComponentID id); // the data will be copied over
        ComponentArrayBuilder &AddComponent(const SerializedComponent &sc); // the data will be copied over

        template <typename T, typename = std::enable_if_t<T::IsUnique()>> ComponentArrayBuilder &AddComponent(const T &component)
        {
            SerializedComponent sc;
            sc.isUnique = true;
            sc.isTag = T::IsTag();
            sc.type = T::GetTypeId();
            if constexpr (T::IsTag() == false)
            {
                sc.alignmentOf = alignof(T);
                sc.sizeOf = sizeof(T);
                sc.data = (ui8 *)&component;
            }
            return AddComponent(sc);
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T> == false>, typename = void> ComponentArrayBuilder &AddComponent(const T &)
        {
            static_assert(false, "Passed value is not a component");
        }

        template <typename T, typename = std::enable_if_t<T::IsUnique() == false && T::IsTag() == false>> ComponentArrayBuilder &AddComponent(const T &component, ComponentID id = {})
        {
            SerializedComponent sc;
            sc.alignmentOf = alignof(T);
            sc.sizeOf = sizeof(T);
            sc.isUnique = false;
            sc.isTag = false;
            sc.type = T::GetTypeId();
            sc.data = (ui8 *)&component;
            sc.id = id;
            return AddComponent(sc);
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T> == false>, typename = void> ComponentArrayBuilder &AddComponent(const T &, ComponentID = {})
        {
            static_assert(false, "Passed value is not a component");
        }
    };
}