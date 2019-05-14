#pragma once

#include "TypeIdentifiable.hpp"
#include "Component.hpp"

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
            } _u{};
        };

    #ifdef DEBUG
        friend class ArchetypeReflector;
        vector<StableTypeId> _storedTypes{};
    #endif

    public:
        template <typename T, typename E = T, StableTypeId E::*type = nullptr> [[nodiscard]] static Archetype Create(Array<const T> types)
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
                    result._u.typePart ^= (ui32)tType.Hash();

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
            } _u{};
        };

    #ifdef DEBUG
        friend class ArchetypeReflector;
        vector<StableTypeId> _storedTypes{};
        vector<StableTypeId> _storedTypesFull{};
    #endif

    public:
        template <typename T, typename E, StableTypeId E::*type, ComponentID T::*id> [[nodiscard]] static ArchetypeFull Create(Array<const T> types)
        {
            ArchetypeFull result;
            
            auto shor = Archetype::Create<T, E, type>(types);
            result._u.typePart = shor._u.typePart;

        #ifdef DEBUG
            result._storedTypes = move(shor._storedTypes);
            for (const auto &t : types)
            {
                result._storedTypesFull.push_back(t.*type);
            }
            std::sort(result._storedTypesFull.begin(), result._storedTypesFull.end());
        #endif
            
            for (const T &t : types)
            {
                result._u.idPart ^= Hash::Integer((t.*id).ID());
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
                return hash && 0xFFFFFFFF;
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
                return hash & 0xFFFFFFFF;
            }
            else
            {
                return value.Hash();
            }
        }
    };
}