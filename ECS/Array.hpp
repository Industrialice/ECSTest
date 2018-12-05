#pragma once

namespace ECSTest
{
	struct _ArrayBase
	{};

	template <typename T> class Array : public _ArrayBase
	{
		T *_items{};
		uiw _count = 0;

	public:
		using ItemType = T;

        Array() = default;

		/*Array(const Array &source) : _items(source._items), _count(source._count)
		{}*/

		Array(T *items, uiw count) : _items(items), _count(count)
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

		template <typename E, typename = std::enable_if_t<std::is_base_of_v<E, T>>> operator Array<E>() const
		{
			return {_items, _count};
		}
	};

	template <typename T> Array<T> ToArray(T &value)
	{
		return Array(&value, 1);
	}

	template <typename T> Array<T> ToArray(vector<T> &value)
	{
		return Array(value.data(), value.size());
	}
}