#pragma once

#include <SystemCreation.hpp>

namespace ECSEngine
{
	struct SetInitialPositionSystem : IndirectSystem<SetInitialPositionSystem>
	{
		void Accept(Array<Position> &positions, Array<Rotation> &rotations, SubtractiveComponent<Camera>, RequiredComponent<Physics>, RequiredComponentAny<BoxCollider, SphereCollider, CapsuleCollider, MeshCollider>) {}
		
		virtual void Update(Environment &env) override
		{
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) override
		{
			for (const auto &entry : stream)
			{
				_initialPositions[entry.entityID] = {entry.GetComponent<Position>(), entry.GetComponent<Rotation>()};
			}
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) override
		{
			for (const auto &entry : stream)
			{
				_initialPositions[entry.entityID] = {entry.GetComponent<Position>(), entry.GetComponent<Rotation>()};
			}
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
		{
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) override
		{
			for (const auto &entry : stream.Enumerate<Position>())
			{
				_initialPositions.erase(entry.entityID);
			}
			for (const auto &entry : stream.Enumerate<Rotation>())
			{
				_initialPositions.erase(entry.entityID);
			}
		}
		
		virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) override
		{
			for (auto id : stream)
			{
				_initialPositions.erase(id);
			}
		}
		
		virtual void ControlInput(Environment &env, const ControlAction &action) override
		{
			if (auto key = action.Get<ControlAction::Key>(); key)
			{
				if (key->key == KeyCode::Space && key->keyState == ControlAction::Key::KeyState::Pressed)
				{
					env.messageBuilder.ComponentChangedHint(Position::Description(), _initialPositions.size());
					env.messageBuilder.ComponentChangedHint(Rotation::Description(), _initialPositions.size());
					for (auto &[id, posrot] : _initialPositions)
					{
						env.messageBuilder.ComponentChanged(id, posrot.first);
						env.messageBuilder.ComponentChanged(id, posrot.second);
					}
				}
			}
		}

	private:
		std::unordered_map<EntityID, pair<Position, Rotation>> _initialPositions{};
	};
}