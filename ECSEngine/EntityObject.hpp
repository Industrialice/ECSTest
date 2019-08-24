#pragma once

namespace ECSEngine
{
	struct EntityObject
	{
		EntityID id{};
		ComponentArrayBuilder components{};
	};
}