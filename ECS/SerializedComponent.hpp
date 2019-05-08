#pragma once

#include "Component.hpp"

namespace ECSTest
{
	struct ComponentDescription
	{
		StableTypeId type{};
		ui16 sizeOf{};
		ui16 alignmentOf{};
		bool isUnique{};
		bool isTag{};
	};

    struct SerializedComponent : ComponentDescription
    {
        const ui8 *data{}; // aigned by alignmentOf
        ComponentID id{};

        template <typename T, typename = enable_if_t<T::IsTag() == false>> [[nodiscard]] T &Cast()
        {
            ASSUME(T::GetTypeId() == type);
            return *(T *)data;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> [[nodiscard]] const T &Cast() const
        {
            ASSUME(T::GetTypeId() == type);
            return *(T *)data;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> [[nodiscard]] T *TryCast()
        {
            if (T::GetTypeId() == type)
            {
                return (T *)data;
            }
            return nullptr;
        }

        template <typename T, typename = enable_if_t<T::IsTag() == false>> [[nodiscard]] const T *TryCast() const
        {
            if (T::GetTypeId() == type)
            {
                return (T *)data;
            }
            return nullptr;
        }

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