#include "PreHeader.hpp"
#include "World.hpp"

using namespace ECSTest;

void World::AddEntity(unique_ptr<Entity> entity)
{
	ASSUME(entity->ID().IsValid());
    _entities.push_back(move(entity));
}

UniqueIdManager &World::IDManager()
{
	return _idManager;
}

const UniqueIdManager &World::IDManager() const
{
	return _idManager;
}
