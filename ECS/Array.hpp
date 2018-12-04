#pragma once

namespace ECSTest
{
	struct _ArrayBase
	{};

	template <typename T> class Array : public _ArrayBase
	{
		T *const _items{};
		const uiw _count = 0;

	public:
		using ItemType = T;

        Array() = default;

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
	};
}