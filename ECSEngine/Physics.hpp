#pragma once

#include <AssetId.hpp>

namespace ECSEngine
{
	struct PhysicsProperties
	{
		enum MotionControl { Others, OthersAndGravity, Kinematic };

		optional<f32> mass = nullopt; // 0 means infinity mass, if mass is not set it'll be computed based on physical material and attached colliders
		f32 linearDamping = 0.001f; // 0 means infinity inertia
		f32 angularDamping = 0.001f; // 0 means infinity inertia
		MotionControl motionControl = MotionControl::OthersAndGravity;
		boolVector3 lockPositionAxis = {false, false, false};
		boolVector3 lockRotationAxis = {false, false, false};
		optional<Vector3> centerOfMass = nullopt; // if not set it will be computed based on attached colliders
		optional<Vector3> inertiaTensor = nullopt; // if not set it will be computed based on attached colliders
		optional<Quaternion> inertiaTensorRotation = nullopt; // if not set it will be computed based on attached colliders
		optional<ui32> solverIterations = nullopt; // when not set the value from the physics settings will be used
		optional<ui32> solverVelocityIterations = nullopt; // when not set the value from the physics settings will be used
		optional<f32> maxAngularVelocity = nullopt; // when not set the value from the physics settings will be used, 0 means infinity velocity
		optional<f32> sleepThreshold = nullopt; // when not set the value from the physics settings will be used
		optional<f32> wakeCounter = nullopt; // when not set the value from the physics settings will be used
		optional<f32> contactOffset = nullopt; // when not set the value from the physics settings will be used
		optional<f32> restOffset = nullopt; // when not set the value from the physics settings will be used)
	};

	struct PhysicsAssetId : AssetId
	{
		using AssetId::AssetId;
	};

	struct PhysicsAsset : TypeIdentifiable<PhysicsAsset>
	{
		using assetIdType = PhysicsAssetId;
		PhysicsProperties data{};
	};
}