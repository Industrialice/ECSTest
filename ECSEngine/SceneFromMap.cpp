#include "PreHeader.hpp"
#include "SceneFromMap.hpp"
#include <EntitiesStreamBuilder.hpp>
#include "Components.hpp"

using namespace ECSEngine;

static void AddObjectsFromMap(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream);
static void ParseObjectIntoEntitiesStream(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream);
template <typename T = Vector3> T ReadVec3(string_view source);
template <typename T = Vector4> T ReadVec4(string_view source);

unique_ptr<IEntitiesStream> SceneFromMap::Create(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper)
{
	auto stream = make_unique<EntitiesStream>();

	AddObjectsFromMap(pathToMap, pathToMapAssets, idGenerator, assetIdMapper, *stream);

	//{
	//	EntitiesStream::EntityData entity;

	//	Position pos;
	//	entity.AddComponent(pos);

	//	Rotation rot;
	//	entity.AddComponent(rot);

	//	Window window;
	//	window.height = 2160;
	//	window.width = 3840;
	//	window.isFullscreen = false;
	//	window.isMaximized = false;
	//	window.isNoBorders = false;
	//	strcpy_s(window.title.data(), window.title.size(), "Industrialice ECS test engine");
	//	window.x = (GetSystemMetrics(SM_CXSCREEN) - window.width) / 2;
	//	window.y = (GetSystemMetrics(SM_CYSCREEN) - window.height) / 2;
	//	window.cursorType = Window::CursorTypet::Normal;

	//	RT rt;
	//	rt.target = window;

	//	ClearColor clearColor;
	//	clearColor.color = ColorR8G8B8(0, 0, 0);

	//	Camera camera;
	//	camera.rt[0] = rt;
	//	camera.farPlane = FLT_MAX;
	//	camera.nearPlane = 0.1f;
	//	camera.fov = 75.0f;
	//	camera.isClearDepthStencil = true;
	//	camera.depthBufferFormat = Camera::DepthBufferFormat::DepthOnly;
	//	camera.projectionType = Camera::ProjectionTypet::Perspective;
	//	camera.clearWith = clearColor;
	//	entity.AddComponent(camera);

	//	stream->AddEntity(idGenerator.Generate(), move(entity));
	//}

	return move(stream);
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

	string name;

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

		if (key == "name")
		{
			name = value;
		}
		else if (key == "position")
		{
			Position pos;
			pos.position = ReadVec3(value);
			entity.AddComponent(pos);
		}
		else if (key == "rotation")
		{
			Rotation rot;
			rot.rotation = ReadVec4<Quaternion>(value);
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
			Scale scale;
			scale.scale = ReadVec3(value);

			if (!EqualsWithEpsilon(scale.scale, {1, 1, 1}))
			{
				entity.AddComponent(scale);
			}
		}
		else if (key == "camera")
		{
			Window window;
			window.height = 2160;
			window.width = 3840;
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
		}

		current = current.substr(endOfLine + 1);
		while (current.starts_with(' '))
		{
			current = current.substr(1);
		}
	}

	stream.AddEntity(idGenerator.Generate(), move(entity));
}

template<typename T> T ReadVec3(string_view source)
{
	T result;

	string_view xStr = source;
	uiw xEnd = xStr.find(',');
	if (xEnd == string_view::npos)
	{
		SOFTBREAK;
		return{};
	}
	if (auto [pointer, error] = std::from_chars(xStr.data(), xStr.data() + xEnd, result.x); error != std::errc())
	{
		SOFTBREAK;
		return{};
	}

	string_view yStr = xStr.substr(xEnd + 1);
	uiw yEnd = yStr.find(',');
	if (yEnd == string_view::npos)
	{
		SOFTBREAK;
		return{};
	}
	if (auto [pointer, error] = std::from_chars(yStr.data(), yStr.data() + yEnd, result.y); error != std::errc())
	{
		SOFTBREAK;
		return{};
	}

	string_view zStr = yStr.substr(yEnd + 1);
	if (auto [pointer, error] = std::from_chars(zStr.data(), zStr.data() + zStr.size(), result.z); error != std::errc())
	{
		SOFTBREAK;
		return{};
	}

	return result;
}

template<typename T> T ReadVec4(string_view source)
{
	T result;

	string_view xStr = source;
	uiw xEnd = xStr.find(',');
	if (xEnd == string_view::npos)
	{
		SOFTBREAK;
		return {};
	}
	if (auto [pointer, error] = std::from_chars(xStr.data(), xStr.data() + xEnd, result.x); error != std::errc())
	{
		SOFTBREAK;
		return {};
	}

	string_view yStr = xStr.substr(xEnd + 1);
	uiw yEnd = yStr.find(',');
	if (yEnd == string_view::npos)
	{
		SOFTBREAK;
		return {};
	}
	if (auto [pointer, error] = std::from_chars(yStr.data(), yStr.data() + yEnd, result.y); error != std::errc())
	{
		SOFTBREAK;
		return {};
	}

	string_view zStr = yStr.substr(yEnd + 1);
	uiw zEnd = zStr.find(',');
	if (zEnd == string_view::npos)
	{
		SOFTBREAK;
		return {};
	}
	if (auto [pointer, error] = std::from_chars(zStr.data(), zStr.data() + zEnd, result.z); error != std::errc())
	{
		SOFTBREAK;
		return {};
	}

	string_view wStr = zStr.substr(zEnd + 1);
	if (auto [pointer, error] = std::from_chars(wStr.data(), wStr.data() + wStr.size(), result.w); error != std::errc())
	{
		SOFTBREAK;
		return {};
	}

	return result;
}
