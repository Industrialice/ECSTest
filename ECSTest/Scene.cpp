#include "PreHeader.hpp"
#include "Scene.hpp"

using namespace ECSTest;

void Scene::AddEntity(unique_ptr<Entity> entity)
{
    _entities.push_back(move(entity));
}
