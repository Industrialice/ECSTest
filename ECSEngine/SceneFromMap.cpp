#include "PreHeader.hpp"
#include "SceneFromMap.hpp"
#include <EntitiesStreamBuilder.hpp>
#include "Components.hpp"

using namespace ECSEngine;

static void AddCamera(EntityIDGenerator &idGenerator, EntitiesStream &stream);
static void AddObjectsFromMap(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream);
static void ParseObjectIntoEntitiesStream(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream);

unique_ptr<IEntitiesStream> SceneFromMap::Create(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper)
{
	auto stream = make_unique<EntitiesStream>();

	AddCamera(idGenerator, *stream);
	AddObjectsFromMap(pathToMap, pathToMapAssets, idGenerator, assetIdMapper, *stream);

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

void AddObjectsFromMap(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream)
{
	Error<> fileError;
	File mapFile(pathToMap, FileOpenMode::OpenExisting, FileProcModes::Read, 0, {}, {}, &fileError);
	if (fileError)
	{
		SOFTBREAK;
		return;
	}

	MemoryMappedFile mapMapped(mapFile);
	if (!mapFile)
	{
		SOFTBREAK;
		return;
	}

	for (const char *objectStart = strchr(reinterpret_cast<const char *>(mapMapped.CMemory()), '['); objectStart; objectStart = strchr(objectStart, '['))
	{
		++objectStart;

		const char *objectEnd = strchr(objectStart, ']');
		if (!objectEnd)
		{
			SOFTBREAK;
			return;
		}

		ParseObjectIntoEntitiesStream(string_view{objectStart, static_cast<uiw>(objectEnd - objectStart + 1)}, pathToMapAssets, idGenerator, assetIdMapper, stream);
	}
}

void ParseObjectIntoEntitiesStream(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream)
{
	EntitiesStream::EntityData entity;

	for (auto current = object; current.size(); )
	{
		uiw delim = current.find(':');
		if (delim == string_view::npos)
		{
			SOFTBREAK;
			return;
		}

		uiw endOfLine = std::min(current.find('\n', delim + 1), current.find(']', delim + 1));
		if (endOfLine == string_view::npos)
		{
			SOFTBREAK;
			return;
		}

		string_view key = current.substr(0, delim);
		string_view value = current.substr(delim + 1, endOfLine - (delim + 1));

		if (key == "position")
		{
			Position pos;

			string_view xStr = value;
			uiw xEnd = xStr.find(',');
			if (xEnd == string_view::npos)
			{
				SOFTBREAK;
				return;
			}
			if (auto [pointer, error] = std::from_chars(xStr.data(), xStr.data() + xEnd, pos.position.x); error != std::errc())
			{
				SOFTBREAK;
				return;
			}

			string_view yStr = xStr.substr(xEnd + 1);
			uiw yEnd = yStr.find(',');
			if (yEnd == string_view::npos)
			{
				SOFTBREAK;
				return;
			}
			if (auto [pointer, error] = std::from_chars(yStr.data(), yStr.data() + yEnd, pos.position.y); error != std::errc())
			{
				SOFTBREAK;
				return;
			}

			string_view zStr = yStr.substr(yEnd + 1);
			if (auto [pointer, error] = std::from_chars(zStr.data(), zStr.data() + zStr.size(), pos.position.z); error != std::errc())
			{
				SOFTBREAK;
				return;
			}

			pos.position *= 100; // TODO: wtf

			entity.AddComponent(pos);
		}
		else if (key == "rotation")
		{
			Vector3 euler;
			Rotation rot;

			string_view xStr = value;
			uiw xEnd = xStr.find(',');
			if (xEnd == string_view::npos)
			{
				SOFTBREAK;
				return;
			}
			if (auto [pointer, error] = std::from_chars(xStr.data(), xStr.data() + xEnd, euler.x); error != std::errc())
			{
				SOFTBREAK;
				return;
			}

			string_view yStr = xStr.substr(xEnd + 1);
			uiw yEnd = yStr.find(',');
			if (yEnd == string_view::npos)
			{
				SOFTBREAK;
				return;
			}
			if (auto [pointer, error] = std::from_chars(yStr.data(), yStr.data() + yEnd, euler.y); error != std::errc())
			{
				SOFTBREAK;
				return;
			}

			string_view zStr = yStr.substr(yEnd + 1);
			if (auto [pointer, error] = std::from_chars(zStr.data(), zStr.data() + zStr.size(), euler.z); error != std::errc())
			{
				SOFTBREAK;
				return;
			}

			rot.rotation = Quaternion::FromEuler(euler);

			entity.AddComponent(rot);
		}
		else if (key == "meshrenderer")
		{
			MeshRenderer meshRenderer;
			meshRenderer.mesh = assetIdMapper.Register<MeshAsset>(pathToMapAssets + FilePath::FromChar(value));
			entity.AddComponent(meshRenderer);
		}
		else if (key == "children")
		{
		}
		else if (key == "scale")
		{
		}

		current = current.substr(endOfLine + 1);
		while (current.starts_with(' '))
		{
			current = current.substr(1);
		}
	}

	stream.AddEntity(idGenerator.Generate(), move(entity));
}
