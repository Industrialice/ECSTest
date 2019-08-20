#include "PreHeader.hpp"
#include "SceneFromMap.hpp"
#include <EntitiesStreamBuilder.hpp>
#include "Components.hpp"
#include "AssetsIdentification.hpp"

using namespace ECSEngine;

static void AddObjectsFromMap(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream);
static void ParseObjectIntoEntitiesStream(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream);
static void ParseSubobjectIntoEntity(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream::EntityData &entity);
static shared_ptr<MeshPathAssetIdentification> ReadMeshRenderer(const FilePath &pathPrepend, string_view data);
template <typename T = Vector3> T ReadVec3(string_view source);
template <typename T = Vector4> T ReadVec4(string_view source);
static optional<string_view> FindObjectBoundaries(string_view source); // returns boundaries without <>

unique_ptr<IEntitiesStream> SceneFromMap::Create(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper)
{
	auto stream = make_unique<EntitiesStream>();

	AddObjectsFromMap(pathToMap, pathToMapAssets, idGenerator, assetIdMapper, *stream);

	//{
	//	EntitiesStream::EntityData entity;

	//	Position pos;
	//	pos.position = {0, 75, 0};
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

	if (!mapFile.Size())
	{
		SOFTBREAK;
		return;
	}

	const char *dataStart = reinterpret_cast<const char *>(mapMapped.CMemory());
	const char *dataEnd = reinterpret_cast<const char *>(mapMapped.CMemory() + mapMapped.Size());

	vector<char> firstStep(dataEnd - dataStart + 1);
	char *target = firstStep.data();

	string_view source{dataStart, static_cast<uiw>(dataEnd - dataStart)};

	while (source.size())
	{
		while (source.starts_with(' '))
		{
			source = source.substr(1);
		}

		uiw end = source.find('\n');
		if (end != string_view::npos)
		{
			++end;
			MemOps::Copy(target, source.data(), end);
			target += end;
			source = source.substr(end);
		}
		else
		{
			MemOps::Copy(target, source.data(), source.size());
			target += source.size();
			if (!source.ends_with('\n'))
			{
				*target = '\n';
				++target;
			}
			source = {};
		}
	}

	dataStart = firstStep.data();
	dataEnd = target;

	do
	{
		auto boundaries = FindObjectBoundaries({dataStart, static_cast<uiw>(dataEnd - dataStart)});
		if (!boundaries)
		{
			break;
		}

		if (!boundaries->starts_with("gameobject:"))
		{
			SOFTBREAK;
			return;
		}

		uiw newLine = boundaries->find('\n');
		if (newLine == string_view::npos)
		{
			SOFTBREAK;
			return;
		}

		*boundaries = boundaries->substr(newLine + 1);

		ParseObjectIntoEntitiesStream(*boundaries, pathToMapAssets, idGenerator, assetIdMapper, stream);

		dataStart = boundaries->data() + boundaries->size() + 2;
	} while (dataStart < dataEnd);
}

void ParseObjectIntoEntitiesStream(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream)
{
	EntitiesStream::EntityData entity;

	for (auto current = object; current.size(); )
	{
		if (current.starts_with("<"))
		{
			auto boundaries = FindObjectBoundaries(current);
			if (!boundaries)
			{
				SOFTBREAK;
				return;
			}
			ParseSubobjectIntoEntity(*boundaries, pathToMapAssets, idGenerator, assetIdMapper, entity);
			const char *boundariesEnd = boundaries->data() + boundaries->size();
			const char *currentEnd = current.data() + current.size();
			current = string_view{boundariesEnd + 2,  static_cast<uiw>(currentEnd - boundariesEnd - 2)};
			continue;
		}

		uiw delim = current.find(':');
		if (delim == string_view::npos)
		{
			SOFTBREAK;
			return;
		}

		uiw endOfLine = std::min(current.find('\n', delim + 1), current.size());

		string_view key = current.substr(0, delim);
		string_view value = current.substr(delim + 1, endOfLine - (delim + 1));

		if (key == "name")
		{
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
		else
		{
			SOFTBREAK;
		}

		current = current.substr(std::min(endOfLine + 1, current.size()));
	}

	stream.AddEntity(idGenerator.Generate(), move(entity));
}

void ParseSubobjectIntoEntity(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream::EntityData &entity)
{
	uiw delim = object.find(':');
	if (delim == string_view::npos)
	{
		SOFTBREAK;
		return;
	}

	string_view subObjectName = object.substr(0, delim);
	string_view subObjectFields = object.substr(std::min(delim + 2, object.size()));

	if (subObjectName == "meshrenderer" || subObjectName == "terrain")
	{
		MeshRenderer meshRenderer;
		meshRenderer.mesh = assetIdMapper.Register<MeshAsset>(ReadMeshRenderer(pathToMapAssets, subObjectFields));
		entity.AddComponent(meshRenderer);
	}
	else if (subObjectName == "camera")
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
	else
	{
		SOFTBREAK;
		return;
	}
}

shared_ptr<MeshPathAssetIdentification> ReadMeshRenderer(const FilePath &pathPrepend, string_view data)
{
	string_view assetPath;
	f32 globalScale = 1.0f;
	ui32 subMeshIndex = 0;
	bool isUseFileScale;

	while (data.size())
	{
		uiw delim = data.find(':');
		if (delim == string_view::npos)
		{
			SOFTBREAK;
			return {};
		}

		uiw endOfLine = std::min(data.find('\n', delim + 1), data.size());

		string_view key = data.substr(0, delim);
		string_view value = data.substr(delim + 1, endOfLine - (delim + 1));

		if (key == "path")
		{
			assetPath = value;
		}
		else if (key == "meshBaseIndex")
		{
			if (auto [pointer, error] = std::from_chars(value.data(), value.data() + value.size(), subMeshIndex); error != std::errc())
			{
				SOFTBREAK;
			}
		}
		else if (key == "globalScale")
		{
			if (auto [pointer, error] = std::from_chars(value.data(), value.data() + value.size(), globalScale); error != std::errc())
			{
				SOFTBREAK;
			}
		}
		else if (key == "isUseFileScale")
		{
			ui32 isUseFileScaleInteger;
			if (auto [pointer, error] = std::from_chars(value.data(), value.data() + value.size(), isUseFileScaleInteger); error != std::errc())
			{
				SOFTBREAK;
			}
			else
			{
				isUseFileScale = isUseFileScaleInteger != 0;
			}
		}
		else
		{
			SOFTBREAK;
		}

		data = data.substr(std::min(endOfLine + 1, data.size()));
	}

	return MeshPathAssetIdentification::New(pathPrepend + FilePath::FromChar(assetPath), subMeshIndex, globalScale, isUseFileScale);
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

optional<string_view> FindObjectBoundaries(string_view source)
{
	uiw startBracket = source.find('<');
	if (startBracket == string_view::npos)
	{
		SOFTBREAK;
		return {};
	}

	const char *sourceEnd = source.data() + source.size();

	const char *objectStart = source.data() + startBracket;

	++objectStart;
	const char *objectEnd = objectStart;
	uiw innerBrackets = 0;

	for (; objectEnd < sourceEnd; ++objectEnd)
	{
		if (*objectEnd == '<')
		{
			++innerBrackets;
		}
		else if (*objectEnd == '>')
		{
			if (innerBrackets == 0)
			{
				break;
			}
			--innerBrackets;
		}
	}

	if (objectEnd == sourceEnd)
	{
		SOFTBREAK;
		return {};
	}

	return string_view{objectStart, static_cast<uiw>(objectEnd - objectStart)};
}