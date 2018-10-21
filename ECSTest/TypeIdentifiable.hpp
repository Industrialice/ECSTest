#pragma once

namespace ECSTest
{
	class TypeId
	{
		const char *_id{};

	public:
		using InternalIdType = decltype(_id);

		constexpr TypeId(InternalIdType id) : _id(id)
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

		[[nodiscard]] constexpr InternalIdType InternalId() const
		{
			return _id;
		}

		[[nodiscard]] ui64 Hash() const
		{
			return Hash::FNVHash<ui64>(_id);
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

    template <TypeId::InternalIdType id, bool isWriteable> struct TypeIdToType;
}

#define GENERATE_TYPE_ID_TO_TYPE(T) \
    template <> struct TypeIdToType<T::GetTypeId().InternalId(), true> \
    { \
        using type = T; \
    }; \
    template <> struct TypeIdToType<T::GetTypeId().InternalId(), false> \
    { \
        using type = const T; \
    }