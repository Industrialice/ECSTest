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

		[[nodiscard]] pair<T *, uiw> GetItems()
		{
			return {_items, _count};
		}

		[[nodiscard]] pair<const T *, uiw> GetItems() const
		{
			return {_items, _count};
		}

		[[nodiscard]] uiw size() const
		{
			return _count;
		}

        [[nodiscard]] T &front()
        {
            ASSUME(_count > 0);
            return _items[0];
        }

        [[nodiscard]] const T &front() const
        {
            ASSUME(_count > 0);
            return _items[0];
        }

        [[nodiscard]] T &back()
        {
            ASSUME(_count > 0);
            return _items[_count - 1];
        }

        [[nodiscard]] const T &back() const
        {
            ASSUME(_count > 0);
            return _items[_count - 1];
        }

        [[nodiscard]] T *begin()
        {
            return _items;
        }

        [[nodiscard]] const T *begin() const
        {
            return _items;
        }

        [[nodiscard]] const T *cbegin() const
        {
            return _items;
        }

        [[nodiscard]] T *end()
        {
            return _items + _count;
        }

        [[nodiscard]] const T *end() const
        {
            return _items + _count;
        }

        [[nodiscard]] const T *cend() const
        {
            return _items + _count;
        }

        [[nodiscard]] T &operator [] (uiw index)
        {
            ASSUME(index < _count);
            return _items[index];
        }

        [[nodiscard]] const T &operator [] (uiw index) const
        {
            ASSUME(index < _count);
            return _items[index];
        }

		T *find(const T &value)
		{
			return std::find(begin(), end(), value);
		}

		const T *find(const T &value) const
		{
			return std::find(begin(), end(), value);
		}

		template <typename E, typename = std::enable_if_t<std::is_base_of_v<E, T>>> operator Array<E>() const
		{
			return {_items, _count};
		}
	};

	template <typename T> Array<T> ToArray(T &value)
	{
		return {&value, 1};
	}

    template <typename T> Array<T> ToArray(T *value, uiw count)
    {
        return {value, count};
    }

    template <typename T> Array<const T> ToArray(const T *value, uiw count)
    {
        return {value, count};
    }

	template <typename T> Array<T> ToArray(vector<T> &value)
	{
		return {value.data(), value.size()};
	}

	template <typename T> Array<const T> ToArray(vector<const T> &value)
	{
		return {value.data(), value.size()};
	}

	template <typename T> Array<const T> ToArray(const vector<T> &value)
	{
		return {value.data(), value.size()};
	}

	template <typename T> Array<const T> ToArray(const vector<const T> &value)
	{
		return {value.data(), value.size()};
	}

	template <typename T, uiw size> Array<T> ToArray(std::array<T, size> &value)
	{
		return {value.data(), size};
	}

	template <typename T, uiw size> Array<const T> ToArray(const std::array<T, size> &value)
	{
		return {value.data(), size};
	}

	template <typename T, uiw size> Array<const T> ToArray(const std::array<const T, size> &value)
	{
		return {value.data(), size};
	}

	template <typename T, uiw size> Array<T> ToArray(T (&value)[size])
	{
		return {value, size};
	}

	template <typename T, uiw size> Array<const T> ToArray(const T (&value)[size])
	{
		return {value, size};
	}

    template <typename T> Array<T> ToArray(Array<T> value)
    {
        return value;
    }
}