#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class ArchetypeFull;

    class Archetype
    {
        friend ArchetypeFull;

        ui64 _hash{};

        static constexpr ui64 MainPartStartBit = 0;
        static constexpr ui64 MainPartMask = 0x0000'0000'FFFF'FFFFULL;

    public:
        Archetype() = default;
        void Add(StableTypeId type);
        void Subtract(StableTypeId type);
        [[nodiscard]] ui64 Hash() const;
        [[nodiscard]] static Archetype FromFull(const ArchetypeFull &source);
        [[nodiscard]] bool operator == (const Archetype &other) const;
        [[nodiscard]] bool operator != (const Archetype &other) const;
        [[nodiscard]] bool operator < (const Archetype &other) const;
    };

    class ArchetypeFull
    {
        friend Archetype;

        ui64 _hash{};

        static constexpr ui64 ExtraPartStartBit = 32;
        static constexpr ui64 ExtraPartMask = 0xFFFF'FFFF'0000'0000ULL;

    public:
        ArchetypeFull() = default;
        void Add(StableTypeId type, ui32 componentID);
        void Subtract(StableTypeId type, ui32 componentID);
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
        size_t operator()(const ECSTest::Archetype &value) const
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
        size_t operator()(const ECSTest::ArchetypeFull &value) const
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