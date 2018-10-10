#include "PreHeader.hpp"
#include "JiggleBonesComponent.hpp"
#include "BulletMoverComponent.hpp"
#include "TransformComponent.hpp"
#include "MeshRendererComponent.hpp"
#include "BoxColliderComponent.hpp"
#include "SoundEmitterComponent.hpp"
#include "PhysicsComponent.hpp"
#include "SystemsManager.hpp"
#include "Entity.hpp"
#include "World.hpp"

using namespace ECSTest;

namespace ECSTest
{
	class ChangeableEntity : public Entity
	{
	public:
		ChangeableEntity(string &&name)
		{
			_name = move(name);
		}

		void SetParent(Entity *parent)
		{
			_parent = parent;
		}

		void AddComponent(unique_ptr<Component> component)
		{
			Entity::AddComponent(move(component));
		}

		void RemoveComponent(const Component &component)
		{
			Entity::RemoveComponent(component);
		}
	};

	class ComponentChanger
	{
	public:
		static void ParentComponent(unique_ptr<Component> component, ChangeableEntity &parent)
		{
			component->_entity = &parent;
			parent.AddComponent(move(component));
		}
	};
}

int main()
{
	StdLib::Initialization::PlatformAbstractionInitialize({});

	World world{};

	{
		auto lightPole = make_unique<ChangeableEntity>("lightPole");
		{
			auto transform = make_unique<TransformComponent>();
			transform->position = {5, 10, 15};
			transform->mutability = TransformComponent::Mutability::Static;
			ComponentChanger::ParentComponent(move(transform), *lightPole);
		}
		{
			auto meshRenderer = make_unique<MeshRendererComponent>();
			ComponentChanger::ParentComponent(move(meshRenderer), *lightPole);
		}
		{
			auto boxCollider = make_unique<BoxColliderComponent>();
			ComponentChanger::ParentComponent(move(boxCollider), *lightPole);
		}
		world.AddEntity(move(lightPole));
	}

	{
		auto radioBox = make_unique<ChangeableEntity>("radioBox");
		{
			auto transform = make_unique<TransformComponent>();
			transform->position = {1, 2, 3};
			ComponentChanger::ParentComponent(move(transform), *radioBox);
		}
		{
			auto meshRenderer = make_unique<MeshRendererComponent>();
			ComponentChanger::ParentComponent(move(meshRenderer), *radioBox);
		}
		{
			auto audioSource = make_unique<SoundEmitterComponent>();
			ComponentChanger::ParentComponent(move(audioSource), *radioBox);
		}
		{
			auto physics = make_unique<PhysicsComponent>();
			ComponentChanger::ParentComponent(move(physics), *radioBox);
		}
		{
			auto boxCollider = make_unique<BoxColliderComponent>();
			ComponentChanger::ParentComponent(move(boxCollider), *radioBox);
		}
		world.AddEntity(move(radioBox));
	}

	SystemsManager manager;
	manager.Spin(world, {});

	printf("done\n");
	getchar();
    return 0;
}