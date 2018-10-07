#include "PreHeader.hpp"
#include "World.hpp"

using namespace ECSTest;

void World::AddEntity(unique_ptr<Entity> entity)
{
    _entities.push_back(move(entity));
}
