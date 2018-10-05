#pragma once

namespace ECSTest
{
	class TypeId
	{
		const char *_id{};

	public:
		using InternalIdType = const char *;

		constexpr TypeId(const char *id) : _id(id)
		{}

		[[nodiscard]] constexpr bool operator == (const TypeId &other) const
		{
			return _id == other._id;
		}

		[[nodiscard]] constexpr bool operator != (const TypeId &other) const
		{
			return _id == other._id;
		}

		[[nodiscard]] constexpr bool operator < (const TypeId &other) const
		{
			return _id < other._id;
		}

		[[nodiscard]] constexpr bool operator > (const TypeId &other) const
		{
			return _id > other._id;
		}

		[[nodiscard]] constexpr const char *InternalId() const
		{
			return _id;
		}
	};

    template <typename T> class TypeIdentifiable
    {
        static constexpr char var = 0;

    public:
        [[nodiscard]] static constexpr TypeId GetTypeId()
        {
            return &var;
        }
    };
}