#include "PreHeader.hpp"
#include "PhysicsSystem.hpp"

using namespace ECSEngine;

struct PhysXSystem : PhysicsSystem
{
	virtual void Update(Environment &env) override
	{
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityAdded &stream) override
	{
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentAdded &stream) override
	{
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentChanged &stream) override
	{
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamComponentRemoved &stream) override
	{
	}
	
	virtual void ProcessMessages(System::Environment &env, const MessageStreamEntityRemoved &stream) override
	{
	}
	
	virtual void ControlInput(Environment &env, const ControlAction &action) override
	{
	}
};

unique_ptr<PhysicsSystem> PhysicsSystem::New()
{
	return make_unique<PhysXSystem>();
}
