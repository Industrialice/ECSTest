#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct SerializedComponent : ComponentDescription
    {
        const byte *data{}; // aigned by alignmentOf
        ComponentID id{};

        WARNING_PUSH
        WARNING_DISABLE_INCREASES_REQUIRED_ALIGNMENT

        template <typename T, typename = enable_if_t<T::IsTag() == false>> [[nodiscard]] T &Cast()
        {
            ASSUME(T::GetTypeId() == type);
            ASSUME(Funcs::IsAligned(data, alignof(T)));
            return *(T *)data;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> [[nodiscard]] const T &Cast() const
        {
            ASSUME(T::GetTypeId() == type);
            ASSUME(Funcs::IsAligned(data, alignof(T)));
            return *(T *)data;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> [[nodiscard]] T *TryCast()
        {
            if (T::GetTypeId() == type)
            {
                ASSUME(Funcs::IsAligned(data, alignof(T)));
                return (T *)data;
            }
            return nullptr;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> [[nodiscard]] const T *TryCast() const
        {
            if (T::GetTypeId() == type)
            {
                ASSUME(Funcs::IsAligned(data, alignof(T)));
                return (T *)data;
            }
            return nullptr;
        }

        WARNING_POP

        template <typename T, typename = enable_if_t<T::IsTag()>> [[nodiscard]] bool TryCast() const
        {
            if (T::GetTypeId() == type)
            {
                return true;
            }
            return false;
        }
    };
}