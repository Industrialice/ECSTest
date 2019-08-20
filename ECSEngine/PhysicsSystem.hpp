#pragma once

#include <SystemCreation.hpp>
#include "Components.hpp"

namespace ECSEngine
{
	struct PhysicsSystem : public IndirectSystem<PhysicsSystem>
	{
		void Accept(Array<Position> &,
			Array<Rotation> &,
			const Array<Scale> *,
			Array<LinearVelocity> *,
			Array<AngularVelocity> *,
			const Array<BoxCollider> *,
			const Array<SphereCollider> *,
			const Array<CapsuleCollider> *,
			const Array<MeshCollider> *,
			RequiredComponentAny<BoxCollider, SphereCollider, CapsuleCollider, MeshCollider>) {}
	};
}