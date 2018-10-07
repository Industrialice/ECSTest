#include "PreHeader.hpp"
#include "Component.hpp"

using namespace ECSTest;

Entity &Component::Entity()
{
    ASSUME(_entity != nullptr);
    return *_entity;
}

const Entity &Component::Entity() const
{
    ASSUME(_entity != nullptr);
    return *_entity;
}