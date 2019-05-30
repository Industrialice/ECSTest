#pragma once

namespace ECSTest
{
	template <typename T> class Array
	{
		T *_items{};
		uiw _count = 0;

	public:
		using ItemType = T;

		constexpr Array() = default;

		constexpr Array(T *items, uiw count) : _items(items), _count(count)
		{}

		[[nodiscard]] constexpr pair<T *, uiw> GetItems()
		{
			return {_items, _count};
		}

		[[nodiscard]] constexpr pair<const T *, uiw> GetItems() const
		{
			return {_items, _count};
		}

		[[nodiscard]] constexpr uiw size() const
		{
			return _count;
		}

        [[nodiscard]] constexpr bool empty() const
        {
            return _count == 0;
        }

        [[nodiscard]] constexpr T &front()
        {
            ASSUME(_count > 0);
            return _items[0];
        }

        [[nodiscard]] constexpr const T &front() const
        {
            ASSUME(_count > 0);
            return _items[0];
        }

        [[nodiscard]] constexpr T &back()
        {
            ASSUME(_count > 0);
            return _items[_count - 1];
        }

        [[nodiscard]] constexpr const T &back() const
        {
            ASSUME(_count > 0);
            return _items[_count - 1];
        }

        [[nodiscard]] constexpr T *begin()
        {
            return _items;
        }

        [[nodiscard]] constexpr const T *begin() const
        {
            return _items;
        }

        [[nodiscard]] constexpr const T *cbegin() const
        {
            return _items;
        }

        [[nodiscard]] constexpr T *end()
        {
            return _items + _count;
        }

        [[nodiscard]] constexpr const T *end() const
        {
            return _items + _count;
        }

        [[nodiscard]] constexpr const T *cend() const
        {
            return _items + _count;
        }

		[[nodiscard]] constexpr T &at(uiw index)
		{
			ASSUME(index < _count);
			return _items[index];
		}

		[[nodiscard]] constexpr const T &at(uiw index) const
		{
			ASSUME(index < _count);
			return _items[index];
		}

        [[nodiscard]] constexpr T &operator [] (uiw index)
        {
            ASSUME(index < _count);
            return _items[index];
        }

        [[nodiscard]] constexpr const T &operator [] (uiw index) const
        {
            ASSUME(index < _count);
            return _items[index];
        }

		constexpr T *find(const T &value)
		{
			return std::find(begin(), end(), value);
		}

		constexpr const T *find(const T &value) const
		{
			return std::find(begin(), end(), value);
		}

        template <typename Predicate> constexpr T *find_if(Predicate &&predicate)
        {
            return std::find_if(begin(), end(), predicate);
        }

        template <typename Predicate> constexpr const T *find_if(Predicate &&predicate) const
        {
            return std::find_if(begin(), end(), predicate);
        }

		constexpr uiw count(const T &value) const
        {
            return std::count(begin(), end(), value);
        }

        template <typename Predicate> constexpr uiw count_if(Predicate &&predicate) const
        {
            return std::count_if(begin(), end(), predicate);
        }

		template <typename E, typename = enable_if_t<is_base_of_v<E, T>>> constexpr operator Array<E>() const
		{
			return {_items, _count};
		}
	};

	template <typename T> [[nodiscard]] constexpr Array<T> ToArray(T &value)
	{
		return {&value, 1};
	}

    template <typename T> [[nodiscard]] constexpr Array<T> ToArray(T *value, uiw count)
    {
        return {value, count};
    }

    template <typename T> [[nodiscard]] constexpr Array<const T> ToArray(const T *value, uiw count)
    {
        return {value, count};
    }

	template <typename T> [[nodiscard]] constexpr Array<T> ToArray(vector<T> &value)
	{
		return {value.data(), value.size()};
	}

	template <typename T> [[nodiscard]] constexpr Array<const T> ToArray(vector<const T> &value)
	{
		return {value.data(), value.size()};
	}

	template <typename T> [[nodiscard]] constexpr Array<const T> ToArray(const vector<T> &value)
	{
		return {value.data(), value.size()};
	}

	template <typename T> [[nodiscard]] constexpr Array<const T> ToArray(const vector<const T> &value)
	{
		return {value.data(), value.size()};
	}

	template <typename T, uiw size> [[nodiscard]] constexpr Array<T> ToArray(array<T, size> &value)
	{
		return {value.data(), size};
	}

	template <typename T> [[nodiscard]] constexpr Array<T> ToArray(array<T, 0> &value)
	{
		return {(T *)nullptr, 0};
	}

	template <typename T, uiw size> [[nodiscard]] constexpr Array<const T> ToArray(const array<T, size> &value)
	{
		return {value.data(), size};
	}

	template <typename T> [[nodiscard]] constexpr Array<const T> ToArray(const array<T, 0> &value)
	{
		return {(const T *)nullptr, 0};
	}

	template <typename T, uiw size> [[nodiscard]] constexpr Array<const T> ToArray(const array<const T, size> &value)
	{
		return {value.data(), size};
	}

	template <typename T> [[nodiscard]] constexpr Array<const T> ToArray(const array<const T, 0> &value)
	{
		return {(const T *)nullptr, 0};
	}

	template <typename T, uiw size> [[nodiscard]] constexpr Array<T> ToArray(T (&value)[size])
	{
		return {value, size};
	}

	template <typename T, uiw size> [[nodiscard]] constexpr Array<const T> ToArray(const T (&value)[size])
	{
		return {value, size};
	}

    template <typename T> [[nodiscard]] constexpr Array<T> ToArray(Array<T> value)
    {
        return value;
    }
}