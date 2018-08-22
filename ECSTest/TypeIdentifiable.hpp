#pragma once

namespace ECSTest
{
    class TypeId
    {
        const char *const _id{};

    public:
        constexpr TypeId(const char *id) : _id(id)
        {}

        constexpr bool operator == (const TypeId &other) const
        {
            return _id == other._id;
        }

        constexpr bool operator != (const TypeId &other) const
        {
            return _id == other._id;
        }

        constexpr bool operator < (const TypeId &other) const
        {
            return _id < other._id;
        }

        constexpr bool operator > (const TypeId &other) const
        {
            return _id > other._id;
        }
    };

    template <typename T> class TypeIdentifiable
    {
        static constexpr char var = 0;

    public:
        static constexpr TypeId GetTypeId()
        {
            return &var;
        }
    };
}