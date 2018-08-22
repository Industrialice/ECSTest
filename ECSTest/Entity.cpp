#include "PreHeader.hpp"
#include "Entity.hpp"

using namespace ECSTest;

void Entity::AddComponent(unique_ptr<Component> component)
{
    _components.push_back(move(component));
}
