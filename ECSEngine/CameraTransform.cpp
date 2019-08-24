#include "PreHeader.hpp"
#include "CameraTransform.hpp"

using namespace ECSEngine;

CameraTransform::CameraTransform(const Vector3 &position, const Vector3 &rotation) : _position(position), _pitch(rotation.x), _yaw(rotation.y), _roll(rotation.z)
{}

CameraTransform::CameraTransform(const Vector3 &position, const Quaternion &rotation) : CameraTransform(position, rotation.ToEuler())
{}

const Vector3 &CameraTransform::Position() const
{
	return _position;
}

void CameraTransform::Position(const Vector3 &newPosition)
{
	_position = newPosition;
	_viewMatrix = nullopt;
}

Vector3 CameraTransform::Rotation() const
{
	return {_pitch, _yaw, _roll};
}

void CameraTransform::Rotation(const Vector3 &pitchYawRollAngles)
{
	_pitch = pitchYawRollAngles.x;
	_yaw = pitchYawRollAngles.y;
	_roll = pitchYawRollAngles.z;
	_viewMatrix = nullopt;
}

void CameraTransform::Rotation(const Quaternion &rotation)
{
	return Rotation(rotation.ToEuler());
}

f32 CameraTransform::Pitch() const
{
	return _pitch;
}

void CameraTransform::Pitch(f32 angle)
{
	_pitch = angle;
	_viewMatrix = nullopt;
}

f32 CameraTransform::Yaw() const
{
	return _yaw;
}

void CameraTransform::Yaw(f32 angle)
{
	_yaw = angle;
	_viewMatrix = nullopt;
}

f32 CameraTransform::Roll() const
{
	return _roll;
}

void CameraTransform::Roll(f32 angle)
{
	_roll = angle;
	_viewMatrix = nullopt;
}

Vector3 CameraTransform::RightAxis() const
{
	auto rotMatrix = Matrix3x3::CreateRS(Vector3{_pitch, _yaw, _roll});
	return Vector3(1, 0, 0) * rotMatrix;
}

Vector3 CameraTransform::UpAxis() const
{
	auto rotMatrix = Matrix3x3::CreateRS(Vector3{_pitch, _yaw, _roll});
	return Vector3(0, 1, 0) * rotMatrix;
}

Vector3 CameraTransform::ForwardAxis() const
{
	auto rotMatrix = Matrix3x3::CreateRS(Vector3{_pitch, _yaw, _roll});
	return Vector3(0, 0, 1) * rotMatrix;
}

void CameraTransform::RotateAroundRightAxis(f32 pitchAngle)
{
	_pitch = RadNormalize(_pitch + pitchAngle);
	_viewMatrix = nullopt;
}

void CameraTransform::RotateAroundUpAxis(f32 yawAngle)
{
	_yaw = RadNormalize(_yaw + yawAngle);
	_viewMatrix = nullopt;
}

void CameraTransform::RotateAroundForwardAxis(f32 rollAngle)
{
	_roll = RadNormalize(_roll + rollAngle);
	_viewMatrix = nullopt;
}

void CameraTransform::Rotate(const Vector3 &pitchYawRollAngles)
{
	if (pitchYawRollAngles.x)
	{
		RotateAroundRightAxis(pitchYawRollAngles.x);
	}
	if (pitchYawRollAngles.y)
	{
		RotateAroundUpAxis(pitchYawRollAngles.y);
	}
	if (pitchYawRollAngles.z)
	{
		RotateAroundForwardAxis(pitchYawRollAngles.z);
	}
}

void CameraTransform::MoveAlongRightAxis(f32 shift)
{
	_position += RightAxis() * shift;
	_viewMatrix = nullopt;
}

void CameraTransform::MoveAlongUpAxis(f32 shift)
{
	_position += UpAxis() * shift;
	_viewMatrix = nullopt;
}

void CameraTransform::MoveAlongForwardAxis(f32 shift)
{
	_position += ForwardAxis() * shift;
	_viewMatrix = nullopt;
}

void CameraTransform::Move(const Vector3 &shift)
{
	if (shift.x)
	{
		MoveAlongRightAxis(shift.x);
	}
	if (shift.y)
	{
		MoveAlongUpAxis(shift.y);
	}
	if (shift.z)
	{
		MoveAlongForwardAxis(shift.z);
	}
}

const Matrix4x3 &CameraTransform::ViewMatrix() const
{
	if (!_viewMatrix)
	{
		auto xAxis = RightAxis();
		auto yAxis = UpAxis();
		auto zAxis = ForwardAxis();
		Vector3 row0{xAxis.x, yAxis.x, zAxis.x};
		Vector3 row1{xAxis.y, yAxis.y, zAxis.y};
		Vector3 row2{xAxis.z, yAxis.z, zAxis.z};
		Vector3 pos{-xAxis.Dot(_position), -yAxis.Dot(_position), -zAxis.Dot(_position)};

		*_viewMatrix = Matrix4x3(row0, row1, row2, pos);
	}
	return *_viewMatrix;
}