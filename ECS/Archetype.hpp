#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class ArchetypeFull;

    class Archetype
    {
        friend ArchetypeFull;

        union
        {
            struct
            {
                ui64 idPart : 32;
                ui64 typePart : 32;
            } _parted;
            ui64 _whole{};
        };

    #ifdef DEBUG
        friend class ArchetypeReflector;
        vector<StableTypeId> _storedTypes{};
    #endif

    public:
        template <typename T, StableTypeId T::*type = nullptr> [[nodiscard]] static Archetype Create(Array<const T> types)
        {
            Archetype result;

            auto unduplicated = ALLOCA_TYPED(types.size(), StableTypeId);
            uiw count = 0;

            for (const T &t : types)
            {
                bool isAdd = true;
                StableTypeId tType;

                if constexpr (type == nullptr)
                {
                    tType = t;
                }
                else
                {
                    tType = t.*type;
                }

                for (uiw search = 0; search < count; ++search)
                {
                    if (unduplicated[search] == tType)
                    {
                        isAdd = false;
                        break;
                    }
                }

                if (isAdd)
                {
                    unduplicated[count++] = tType;
                    result._parted.typePart ^= (ui32)tType.Hash();

                #ifdef DEBUG
                    result._storedTypes.push_back(tType);
                #endif
                }
            }

        #ifdef DEBUG
            std::sort(result._storedTypes.begin(), result._storedTypes.end());
        #endif

            return result;
        }

        Archetype() = default;
        [[nodiscard]] ui64 Hash() const;
        [[nodiscard]] static Archetype FromFull(const ArchetypeFull &source);
        [[nodiscard]] bool operator == (const Archetype &other) const;
        [[nodiscard]] bool operator != (const Archetype &other) const;
        [[nodiscard]] bool operator < (const Archetype &other) const;
    };

    class ArchetypeFull
    {
        friend Archetype;

        union
        {
            struct
            {
                ui64 idPart : 32;
                ui64 typePart : 32;
            } _parted;
            ui64 _whole{};
        };

    #ifdef DEBUG
        friend class ArchetypeReflector;
        vector<StableTypeId> _storedTypes{};
    #endif

    public:
        template <typename T, StableTypeId T::*type, ui32 T::*id> [[nodiscard]] static ArchetypeFull Create(Array<const T> types)
        {
            ArchetypeFull result;
            
            auto shor = Archetype::Create<T, type>(types);
            result._parted.typePart = shor._parted.typePart;
        #ifdef DEBUG
            result._storedTypes = move(shor._storedTypes);
        #endif
            
            for (const T &t : types)
            {
                result._parted.idPart += t.*id;
            }

            return result;
        }

        ArchetypeFull() = default;
        [[nodiscard]] ui64 Hash() const;
        [[nodiscard]] Archetype ToShort() const;
        [[nodiscard]] bool operator == (const ArchetypeFull &other) const;
        [[nodiscard]] bool operator != (const ArchetypeFull &other) const;
        [[nodiscard]] bool operator < (const ArchetypeFull &other) const;
    };
}

namespace std
{
    template <> struct hash<ECSTest::Archetype>
    {
        [[nodiscard]] size_t operator()(const ECSTest::Archetype &value) const
        {
            if constexpr (sizeof(size_t) == 4)
            {
                ui64 hash = value.Hash();
                hash ^= hash >> 32;
                return (ui32)hash;
            }
            else
            {
                return value.Hash();
            }
        }
    };

    template <> struct hash<ECSTest::ArchetypeFull>
    {
        [[nodiscard]] size_t operator()(const ECSTest::ArchetypeFull &value) const
        {
            if constexpr (sizeof(size_t) == 4)
            {
                ui64 hash = value.Hash();
                hash ^= hash >> 32;
                return (ui32)hash;
            }
            else
            {
                return value.Hash();
            }
        }
    };
}