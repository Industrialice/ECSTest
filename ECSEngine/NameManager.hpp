#pragma once

namespace ECSEngine
{
	class NameManager
	{
	protected:
		NameManager() = default;

	public:
		static shared_ptr<NameManager> New();

		virtual string_view NameOf(EntityID id) const = 0;
		virtual void NameOf(EntityID id, string_view name) = 0;
	};
}