#pragma once

#include <SystemCreation.hpp>
#include "Components.hpp"
#include "CameraTransform.hpp"

namespace ECSEngine
{
	struct CameraMovementSystem : IndirectSystem<CameraMovementSystem>
	{
		void Accept(Array<Position> &positions, Array<Rotation> &rotations, const Array<Camera> &cameras) {}
		virtual void Update(Environment &env) override;
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) override;
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) override;
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override;
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) override;
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) override;
		virtual void ControlInput(Environment &env, const ControlAction &action) override;

	private:
		CameraTransform _cameraTransform{};
		EntityID _controlledCameraId{};
		bool _isUpdated = false;
	};
}