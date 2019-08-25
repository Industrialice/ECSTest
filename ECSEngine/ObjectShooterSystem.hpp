#pragma once

#include <SystemCreation.hpp>
#include "EntityObject.hpp"

namespace ECSEngine
{
	struct ObjectShooterSystem : IndirectSystem<ObjectShooterSystem>
	{
		ObjectShooterSystem(const EntityObject &referenceEntity) : _referenceEntity(referenceEntity)
		{}

		void Accept(const Array<Position> &positions, const Array<Rotation> &rotations, RequiredComponent<Camera, ActiveCamera>) {}
		
		virtual void Update(Environment &env) override
		{
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) override
		{
			for (const auto &entry : stream)
			{
				_cameraTransform = {entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation};
				_controllingCamera = entry.entityID;
			}
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) override
		{
			for (const auto &entry : stream)
			{
				_cameraTransform = {entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation};
				_controllingCamera = entry.entityID;
			}
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
		{
			for (const auto &entry : stream.Enumerate<Position>())
			{
				if (entry.entityID == _controllingCamera)
				{
					_cameraTransform.Position(entry.component.position);
				}
			}
			for (const auto &entry : stream.Enumerate<Rotation>())
			{
				if (entry.entityID == _controllingCamera)
				{
					_cameraTransform.Rotation(entry.component.rotation);
				}
			}
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) override
		{
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) override
		{
		}
		
		virtual void ControlInput(Environment &env, const ControlAction &action) override
		{
			if (auto key = action.Get<ControlAction::Key>(); key)
			{
				if (key->key == KeyCode::MouseSecondary && key->keyState != ControlAction::Key::KeyState::Released)
				{
					auto id = env.entityIdGenerator.Generate();
					id.DebugName("procedural sphere");
					auto &cab = env.messageBuilder.AddEntity(id);
					for (const auto &r : _referenceEntity.components.GetComponents())
					{
						if (r.type == Position::GetTypeId() || r.type == Rotation::GetTypeId() || r.type == LinearVelocity::GetTypeId())
						{
							continue;
						}
						cab.AddComponent(r);
					}
					cab.AddComponent(Position{.position = _cameraTransform.Position() + _cameraTransform.ForwardAxis() * 2 - _cameraTransform.UpAxis()});
					cab.AddComponent(LinearVelocity{.velocity = _cameraTransform.ForwardAxis() * 25});
					cab.AddComponent(Rotation());
				}
			}
		}

	private:
		EntityID _controllingCamera{};
		CameraTransform _cameraTransform{};
		EntityObject _referenceEntity{};
	};
}