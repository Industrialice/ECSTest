#include "PreHeader.hpp"
#include "Scene.hpp"
#include <EntitiesStreamBuilder.hpp>
#include "Components.hpp"

using namespace ECSEngine;

static void AddCamera(EntityIDGenerator &idGenerator, EntitiesStream &stream);
static void AddObjects(EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, AssetsManager &assetsManager, EntitiesStream &stream);

unique_ptr<IEntitiesStream> Scene::Create(EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, AssetsManager &assetsManager)
{
    auto stream = make_unique<EntitiesStream>();

	AddCamera(idGenerator, *stream);
	AddObjects(idGenerator, assetIdMapper, assetsManager, *stream);

    return move(stream);
}

void AddCamera(EntityIDGenerator &idGenerator, EntitiesStream &stream)
{
	EntitiesStream::EntityData entity;

	Window window;
	window.height = 480;
	window.width = 640;
	window.isFullscreen = false;
	window.isMaximized = false;
	window.isNoBorders = false;
	strcpy_s(window.title.data(), window.title.size(), "Industrialice ECS test engine");
	window.x = (GetSystemMetrics(SM_CXSCREEN) - window.width) / 2;
	window.y = (GetSystemMetrics(SM_CYSCREEN) - window.height) / 2;
	window.cursorType = Window::CursorTypet::Normal;

	RT rt;
	rt.target = window;

	ClearColor clearColor;
	clearColor.color = ColorR8G8B8(0, 0, 0);

	Camera camera;
	camera.rt[0] = rt;
	camera.farPlane = FLT_MAX;
	camera.nearPlane = 0.1f;
	camera.fov = 75.0f;
	camera.isClearDepthStencil = true;
	camera.depthBufferFormat = Camera::DepthBufferFormat::DepthOnly;
	camera.projectionType = Camera::ProjectionTypet::Perspective;
	camera.clearWith = clearColor;
	entity.AddComponent(camera);

	Position position;
	position.position = {0, 0, -5};
	entity.AddComponent(position);

	Rotation rotation;
	rotation.rotation = {};
	entity.AddComponent(rotation);

	stream.AddEntity(idGenerator.Generate(), move(entity));
}

void AddObjects(EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, AssetsManager &assetsManager, EntitiesStream &stream)
{
	MeshAssetId assetId = assetIdMapper.Register<MeshAsset>(L"Assets/Schoolhouse.fbx,0");
	const MeshAsset *loadedAsset = assetsManager.Load<MeshAsset>(assetId);
	if (!loadedAsset)
	{
		SOFTBREAK;
		return;
	}

	const auto &desc = loadedAsset->desc.subMeshInfos[0];

	EntitiesStream::EntityData entity;

	Position position;
	Rotation rotation;
	Scale scale;

	desc.transformation.Decompose(&rotation.rotation, &position.position, &scale.scale);

	entity.AddComponent(position);
	entity.AddComponent(rotation);
	entity.AddComponent(scale);

	MeshRenderer meshRenderer;
	meshRenderer.mesh = assetId;
	meshRenderer.materials = {};
	entity.AddComponent(meshRenderer);

	stream.AddEntity(idGenerator.Generate(), move(entity));
}