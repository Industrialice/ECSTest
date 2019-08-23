#pragma once

#include <SystemCreation.hpp>
#include "Components.hpp"

namespace ECSEngine
{
	struct PhysicsSystemSettings
	{
		uiw workersThreadsCount = 1; // set it to zero to perform simulation only on the system's thread
		Vector3 gravity = {0, -9.81f, 0}; // default gravity, can be adjusted during runtime
		bool enableAdditionalStabilization = false; // improves stability of object stacks but costs performance, it's also an experimental feature that results in loss of momentum, cannot be changed after initialization
		ui32 simulationMemorySize = 16384; // 1024 KB, size of a temporary buffer that holds simulation data, to find the final size multiply by 64
		bool isProcessingOnGPU = false; // if true will try to utilize CUDA for physics simulation
		f32 contactOffset = 0.0075f;
		f32 restOffset = 0.0f;
		f32 sleepThreshold = 0.01f;
		f32 wakeCounter = 0.2f;
		ui32 solverPositionIterations = 4;
		ui32 solverVelocityIterations = 1;
		f32 maxAngularVelocity = 100.0f;
		f32 maxDepenetrationVelocity = 1e+32f;
	};

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
			const Array<Physics> *,
			RequiredComponentAny<BoxCollider, SphereCollider, CapsuleCollider, MeshCollider>) {}

		[[nodiscard]] static unique_ptr<PhysicsSystem> New(const PhysicsSystemSettings &settings);
	};
}