#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct SerializedComponent
    {
        StableTypeId type{};
        ui16 sizeOf{};
        ui16 alignmentOf{};
        const ui8 *data{}; // aigned by alignmentOf
        bool isUnique{};
        bool isTag{};
        ComponentID id{};

        template <typename T, typename = enable_if_t<T::IsTag() == false>> T &Cast()
        {
            ASSUME(T::GetTypeId() == type);
            return *(T *)data;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> const T &Cast() const
        {
            ASSUME(T::GetTypeId() == type);
            return *(T *)data;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> T *TryCast()
        {
            if (T::GetTypeId() == type)
            {
                return (T *)data;
            }
            return nullptr;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> const T *TryCast() const
        {
            if (T::GetTypeId() == type)
            {
                return (T *)data;
            }
            return nullptr;
        }

        template <typename T, typename = enable_if_t<T::IsTag()>> bool TryCast() const
        {
            if (T::GetTypeId() == type)
            {
                return true;
            }
            return false;
        }
    };
}