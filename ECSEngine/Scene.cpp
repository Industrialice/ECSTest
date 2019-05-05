#include "PreHeader.hpp"
#include "Scene.hpp"
#include <EntitiesStreamBuilder.hpp>
#include "Components.hpp"

using namespace ECSEngine;

unique_ptr<IEntitiesStream> Scene::Create(EntityIDGenerator &idGenerator)
{
    auto stream = make_unique<EntitiesStream>();

    EntitiesStream::EntityData entity;

    Window window;
    window.height = 600;
    window.width = 800;
    window.isFullscreen = false;
    window.isMaximized = false;
    window.isNoBorders = false;
    strcpy_s(window.title.data(), window.title.size(), "Industrialice ECS test engine");
    window.x = 500;
    window.y = 500;
    window.cursorType = Window::CursorTypet::Normal;

    RT rt;
    rt.target = window;

    Camera camera;
    camera.rts[0] = rt;
    camera.farPlane = 1000.0f;
    camera.nearPlane = 0.1f;
    camera.fov = 75.0f;
    camera.isClearDepth = false;
    camera.projectionType = Camera::ProjectionType::Perspective;
    entity.AddComponent(camera);

    Position position;
    position.position = {0, 0, -5};
    entity.AddComponent(position);

    Rotation rotation;
    rotation.rotation = {};
    entity.AddComponent(rotation);

    stream->AddEntity(idGenerator.Generate(), move(entity));

    return move(stream);
}
