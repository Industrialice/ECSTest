#include "PreHeader.hpp"
#include "CameraMovementSystem.hpp"

using namespace ECSEngine;

void CameraMovementSystem::Update(Environment &env)
{
	if (!_controlledCameraId)
	{
		return;
	}

	float camMovementScale = env.timeSinceLastFrame * 15;
	if (env.keyController->GetKeyInfo(KeyCode::LShift).IsPressed())
	{
		camMovementScale *= 3;
	}
	if (env.keyController->GetKeyInfo(KeyCode::LControl).IsPressed())
	{
		camMovementScale *= 0.33f;
	}

	if (env.keyController->GetKeyInfo(KeyCode::S).IsPressed())
	{
		_cameraTransform.MoveAlongForwardAxis(-camMovementScale);
		_isUpdated = true;
	}
	if (env.keyController->GetKeyInfo(KeyCode::W).IsPressed())
	{
		_cameraTransform.MoveAlongForwardAxis(camMovementScale);
		_isUpdated = true;
	}
	if (env.keyController->GetKeyInfo(KeyCode::A).IsPressed())
	{
		_cameraTransform.MoveAlongRightAxis(-camMovementScale);
		_isUpdated = true;
	}
	if (env.keyController->GetKeyInfo(KeyCode::D).IsPressed())
	{
		_cameraTransform.MoveAlongRightAxis(camMovementScale);
		_isUpdated = true;
	}
	if (env.keyController->GetKeyInfo(KeyCode::R).IsPressed())
	{
		_cameraTransform.MoveAlongUpAxis(camMovementScale);
		_isUpdated = true;
	}
	if (env.keyController->GetKeyInfo(KeyCode::F).IsPressed())
	{
		_cameraTransform.MoveAlongUpAxis(-camMovementScale);
		_isUpdated = true;
	}

	if (!_isUpdated)
	{
		return;
	}

	Position pos;
	pos.position = _cameraTransform.Position();
	env.messageBuilder.ComponentChanged(_controlledCameraId, pos);

	Rotation rot;
	rot.rotation = Quaternion::FromEuler(_cameraTransform.Rotation());
	env.messageBuilder.ComponentChanged(_controlledCameraId, rot);

	_isUpdated = false;
}

void CameraMovementSystem::ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream)
{
	if (_controlledCameraId)
	{
		SOFTBREAK;
	}

	const auto &entity = *stream.begin();

	_controlledCameraId = entity.entityID;

	_cameraTransform = {entity.GetComponent<Position>().position, entity.GetComponent<Rotation>().rotation};
}

void CameraMovementSystem::ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream)
{
	if (_controlledCameraId)
	{
		return;
	}

	if (stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == Camera::GetTypeId())
	{
		const auto &entity = *stream.begin();

		_controlledCameraId = entity.entityID;

		_cameraTransform = {entity.GetComponent<Position>().position, entity.GetComponent<Rotation>().rotation};
	}
}

void CameraMovementSystem::ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream)
{
	for (const auto &info : stream.Enumerate<Position>())
	{
		if (info.entityID == _controlledCameraId)
		{
			_cameraTransform.Position(info.component.position);
			break;
		}
	}
	for (const auto &info : stream.Enumerate<Rotation>())
	{
		if (info.entityID == _controlledCameraId)
		{
			_cameraTransform.Rotation(info.component.rotation.ToEuler());
			break;
		}
	}
}

void CameraMovementSystem::ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream)
{
	if (stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == Camera::GetTypeId())
	{
		_cameraTransform = {};
		_controlledCameraId = {};
	}
}

void CameraMovementSystem::ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream)
{
	for (EntityID id : stream)
	{
		if (id == _controlledCameraId)
		{
			_cameraTransform = {};
			_controlledCameraId = {};
			break;
		}
	}
}

void CameraMovementSystem::ControlInput(Environment &env, const ControlAction &action)
{
	if (_controlledCameraId)
	{
		if (auto mouse = action.Get<ControlAction::MouseMove>(); mouse)
		{
			if (mouse->delta.x)
			{
				_cameraTransform.RotateAroundUpAxis(mouse->delta.x * 0.001f);
				_isUpdated = true;
			}
			if (mouse->delta.y)
			{
				_cameraTransform.RotateAroundRightAxis(mouse->delta.y * 0.001f);
				_isUpdated = true;
			}
		}
		else if (auto wheel = action.Get<ControlAction::MouseWheel>(); wheel)
		{
			if (wheel->delta)
			{
				_cameraTransform.MoveAlongForwardAxis(-wheel->delta * 5.0f);
				_isUpdated = true;
			}
		}
	}
}