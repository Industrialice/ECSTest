#include "PreHeader.hpp"
#include "PhysicsProperties.hpp"

template <typename T> struct Fields
{
	static i32 Compare(const T &left, const T &right)
	{
		if constexpr (std::is_floating_point_v<T>)
		{
			if (EqualsWithEpsilon(left, right))
			{
				return 0;
			}
		}
		else
		{
			if (left == right)
			{
				return 0;
			}
		}
		if (left < right)
		{
			return -1;
		}
		return 1;
	}
};

template <> struct Fields<boolVector3>
{
	static i32 Compare(const boolVector3 &left, const boolVector3 &right)
	{
		i32 compare = Fields<bool>::Compare(left.x, right.x);
		if (compare) return compare;
		
		compare = Fields<bool>::Compare(left.y, right.y);
		if (compare) return compare;

		return Fields<bool>::Compare(left.z, right.z);
	}
};

template <> struct Fields<Vector3>
{
	static i32 Compare(const Vector3 &left, const Vector3 &right)
	{
		i32 compare = Fields<f32>::Compare(left.x, right.x);
		if (compare) return compare;

		compare = Fields<f32>::Compare(left.y, right.y);
		if (compare) return compare;

		return Fields<f32>::Compare(left.z, right.z);
	}
};

template <> struct Fields<Quaternion>
{
	static i32 Compare(const Quaternion &left, const Quaternion &right)
	{
		i32 compare = Fields<f32>::Compare(left.x, right.x);
		if (compare) return compare;

		compare = Fields<f32>::Compare(left.y, right.y);
		if (compare) return compare;

		compare = Fields<f32>::Compare(left.z, right.z);
		if (compare) return compare;

		return Fields<f32>::Compare(left.w, right.w);
	}
};

template <typename T> struct Fields<optional<T>>
{
	static i32 Compare(const optional<T> &left, const optional<T> &right)
	{
		i32 compare = Fields<bool>::Compare(left.has_value(), right.has_value());
		if (compare) return compare;
		if (left.has_value() == false) return 0;
		return Fields<T>::Compare(*left, *right);
	}
};

bool ECSEngine::PhysicsProperties::operator < (const PhysicsProperties &other) const
{
	#define C(name)	if (i32 compare = Fields<decltype(name)>::Compare(name, other.name); compare) return compare == -1;

	C(mass);
	C(linearDamping);
	C(angularDamping);
	if (ui32 left = static_cast<ui32>(motionControl), right = static_cast<ui32>(other.motionControl); left != right)
	{
		return left < right;
	}
	C(lockPositionAxis);
	C(lockRotationAxis);
	C(centerOfMass);
	C(inertiaTensor);
	C(inertiaTensorRotation);
	C(solverIterations);
	C(solverVelocityIterations);
	C(maxAngularVelocity);
	C(maxDepenetrationVelocity);
	C(sleepThreshold);
	C(wakeCounter);
	C(contactOffset);
	C(restOffset);

	#undef C

	return false;
}

bool ECSEngine::PhysicsProperties::operator == (const PhysicsProperties &other) const
{
	#define C(name)	if (i32 compare = Fields<decltype(name)>::Compare(name, other.name); compare) return false;

	C(mass);
	C(linearDamping);
	C(angularDamping);
	if (ui32 left = static_cast<ui32>(motionControl), right = static_cast<ui32>(other.motionControl); left != right)
	{
		return false;
	}
	C(lockPositionAxis);
	C(lockRotationAxis);
	C(centerOfMass);
	C(inertiaTensor);
	C(inertiaTensorRotation);
	C(solverIterations);
	C(solverVelocityIterations);
	C(maxAngularVelocity);
	C(maxDepenetrationVelocity);
	C(sleepThreshold);
	C(wakeCounter);
	C(contactOffset);
	C(restOffset);

	#undef C

	return true;
}

uiw ECSEngine::PhysicsProperties::Hash() const
{
	uiw hash = 0x811c9dc5u;
	#define H(name) { hash ^= std::hash<decltype(name)>()(name); hash *= 16777619u; }

	H(mass.has_value());
	H(motionControl);
	H(lockPositionAxis);
	H(lockRotationAxis);
	H(centerOfMass.has_value());
	H(inertiaTensor.has_value());
	H(inertiaTensorRotation.has_value());
	H(solverIterations);
	H(solverVelocityIterations);
	H(maxAngularVelocity.has_value());
	H(maxDepenetrationVelocity.has_value());
	H(sleepThreshold.has_value());
	H(wakeCounter.has_value());
	H(contactOffset.has_value());
	H(restOffset.has_value());

	#undef H

	return hash;
}
