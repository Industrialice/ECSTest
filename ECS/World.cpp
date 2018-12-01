#include "PreHeader.hpp"
#include "World.hpp"

using namespace ECSTest;

void World::AddEntity(Entity &&entity, vector<ComponentInfo> &&components)
{
	ASSUME(entity.ID().IsValid());
    _entities.push_back({move(entity), move(components)});
}