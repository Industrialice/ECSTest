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

		[[nodiscard]] uiw Count() const
		{
			return _count;
		}
	};
}