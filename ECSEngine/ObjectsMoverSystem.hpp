#pragma once

#include <SystemCreation.hpp>

namespace ECSEngine
{
	struct ObjectsMoverSystem : DirectSystem<ObjectsMoverSystem>
	{
		void Accept(Array<Rotation> &rotations, RequiredComponent<Position, MeshRenderer>, Environment &env)
		{
			for (auto &rot : rotations)
			{
				rot.rotation = Quaternion::FromEuler({0, env.timeSinceStarted.ToSec(), 0});
			}
		}
	};
}