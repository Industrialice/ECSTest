#pragma once

namespace ECSEngine
{
	class CameraTransform
	{
	private:
		Vector3 _position{};
		f32 _yaw{}, _pitch{}, _roll{};
		// this is just a cached value, so mutable makes sense
		mutable optional<Matrix4x3> _viewMatrix{};

	public:
		CameraTransform() = default;
		CameraTransform(const Vector3 &position, const Vector3 &rotation);
		CameraTransform(const Vector3 &position, const Quaternion &rotation);
		const Vector3 &Position() const;
		void Position(const Vector3 &newPosition);
		Vector3 Rotation() const;
		void Rotation(const Vector3 &pitchYawRollAngles);
		f32 Pitch() const;
		void Pitch(f32 angle);
		f32 Yaw() const;
		void Yaw(f32 angle);
		f32 Roll() const;
		void Roll(f32 angle);
		Vector3 RightAxis() const;
		Vector3 UpAxis() const;
		Vector3 ForwardAxis() const;
		void RotateAroundRightAxis(f32 pitchAngle);
		void RotateAroundUpAxis(f32 yawAngle);
		void RotateAroundForwardAxis(f32 rollAngle);
		void Rotate(const Vector3 &pitchYawRollAngles);
		void MoveAlongRightAxis(f32 shift);
		void MoveAlongUpAxis(f32 shift);
		void MoveAlongForwardAxis(f32 shift);
		void Move(const Vector3 &shift);
		const Matrix4x3 &ViewMatrix() const;
	};
}