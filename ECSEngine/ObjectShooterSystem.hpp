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
			}
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) override
		{
			for (const auto &entry : stream)
			{
				_cameraTransform = {entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation};
			}
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
		{
			for (const auto &entry : stream.Enumerate<Position>())
			{
				_cameraTransform.Position(entry.component.position);
			}
			for (const auto &entry : stream.Enumerate<Rotation>())
			{
				_cameraTransform.Rotation(entry.component.rotation);
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
				if (key->key == KeyCode::MButton2 && key->keyState != ControlAction::Key::KeyState::Released)
				{
					auto id = env.entityIdGenerator.Generate();
					auto &cab = env.messageBuilder.AddEntity(id);
					for (const auto &r : _referenceEntity.components.GetComponents())
					{
						if (r.type == Position::GetTypeId())
						{
							continue;
						}
						cab.AddComponent(r);
					}
					Position pos;
					pos.position = _cameraTransform.Position();
					cab.AddComponent(pos);
				}
			}
		}

	private:
		CameraTransform _cameraTransform{};
		EntityObject _referenceEntity{};
	};
}