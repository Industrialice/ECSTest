#pragma once

#include "TypeIdentifiable.hpp"

namespace ECSTest
{
    class Archetype;

    class ArchetypeShort
    {
        friend Archetype;

        ui64 _hash{};

        static constexpr ui64 MainPartStartBit = 0;
        static constexpr ui64 MainPartMask = 0x0000'FFFF'FFFF'FFFFULL;

    public:
        ArchetypeShort() = default;
        void Add(TypeId type);
        void Subtract(TypeId type);
        [[nodiscard]] ui64 Hash() const;
        [[nodiscard]] static ArchetypeShort FromLong(const Archetype &source);
        [[nodiscard]] bool operator == (const ArchetypeShort &other) const;
        [[nodiscard]] bool operator != (const ArchetypeShort &other) const;
        [[nodiscard]] bool operator < (const ArchetypeShort &other) const;
    };

    class Archetype
    {
        friend ArchetypeShort;

        ui64 _hash{};

        static constexpr ui64 ExtraPartStartBit = 48;
        static constexpr ui64 ExtraPartMask = 0xFFFF'0000'0000'0000ULL;

    public:
        Archetype() = default;
        void Add(TypeId type, ui32 index);
        void Subtract(TypeId type, ui32 index);
        [[nodiscard]] ui64 Hash() const;
        [[nodiscard]] ArchetypeShort ToShort() const;
        [[nodiscard]] bool operator == (const Archetype &other) const;
        [[nodiscard]] bool operator != (const Archetype &other) const;
        [[nodiscard]] bool operator < (const Archetype &other) const;
    };
}

namespace std
{
    template <> struct hash<ECSTest::ArchetypeShort>
    {
        size_t operator()(const ECSTest::ArchetypeShort &value) const
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
}