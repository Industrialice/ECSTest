#include "PreHeader.hpp"
#include "SceneFromMap.hpp"
#include <EntitiesStreamBuilder.hpp>
#include "Components.hpp"
#include "AssetsIdentification.hpp"
#include "EntityObject.hpp"

using namespace ECSEngine;

static Camera CreateDefaultCamera();
static void AddObjectsFromMap(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream, bool &isAddedCamera);
static void ParseObjectIntoEntitiesStream(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, struct KeyValue &keyValueStorage, EntityObject &entity, bool &isAddedCamera);
static void ParseSubobjectIntoEntity(string_view object, const FilePath &pathToMapAssets, AssetIdMapper &assetIdMapper, EntityObject &entity, bool &isAddedCamera, struct KeyValue &keyValueStorage);
static f32 ReadF32(string_view source);
static ui32 ReadUI32(string_view source);
static char ReadChar(string_view source);
template <typename T = Vector3> T ReadVec3(string_view source);
template <typename T = Vector4> T ReadVec4(string_view source);
static optional<string_view> FindObjectBoundaries(string_view source); // returns boundaries without <>

unique_ptr<IEntitiesStream> SceneFromMap::Create(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper)
{
	auto stream = make_unique<EntitiesStream>();

	bool isAddedCamera = false;
	AddObjectsFromMap(pathToMap, pathToMapAssets, idGenerator, assetIdMapper, *stream, isAddedCamera);

	if (!isAddedCamera)
	{
		EntitiesStream::EntityData entity;
		entity.AddComponent(Position{.position = {0, 75, 0}});
		entity.AddComponent(Rotation());
		entity.AddComponent(CreateDefaultCamera());
		entity.AddComponent(ActiveCamera());

		stream->AddEntity(idGenerator.Generate(), move(entity));
	}

	return move(stream);
}

struct KeyValue
{
	struct KeyValueStorage
	{
		string_view key;
		string_view value;
	};

	vector<KeyValueStorage> storage{};

	void Parse(string_view data)
	{
		storage.clear();

		while (data.size())
		{
			uiw delim = data.find(':');
			if (delim == string_view::npos)
			{
				SOFTBREAK;
				return;
			}

			uiw endOfLine = std::min(data.find('\n', delim + 1), data.size());

			string_view key = data.substr(0, delim);
			string_view value = data.substr(delim + 1, endOfLine - (delim + 1));

			storage.push_back({key, value});

			data = data.substr(std::min(endOfLine + 1, data.size()));
		}
	}
};

Camera CreateDefaultCamera()
{
	auto monitors = SystemInfo::MonitorsInfo();
	i32 screenWidth = monitors[0].width;
	i32 screenHeight = monitors[0].height;

	Window window;
	window.height = screenHeight;
	window.width = screenWidth;
	window.isFullscreen = false;
	window.isMaximized = false;
	window.isNoBorders = true;
	strcpy_s(window.title.data(), window.title.size(), "Industrialice ECS test engine");
	window.x = (screenWidth - window.width) / 2;
	window.y = (screenHeight - window.height) / 2;
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

	return camera;
}

void AddObjectsFromMap(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, EntitiesStream &stream, bool &isAddedCamera)
{
	KeyValue keyValueStorage;
	EntityObject entity;

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

	TimeMoment start = TimeMoment::Now();

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

	SENDLOG(Info, SceneFromMap, "Removing spaces took %.2fms\n", (TimeMoment::Now() - start).ToMSec_f64());
	start = TimeMoment::Now();

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

		entity.components.Clear();
		ParseObjectIntoEntitiesStream(*boundaries, pathToMapAssets, idGenerator, assetIdMapper, keyValueStorage, entity, isAddedCamera);

		EntitiesStream::EntityData streamEntity;
		for (const auto &component : entity.components.GetComponents())
		{
			streamEntity.AddComponent(component);
		}
		stream.AddEntity(entity.id, move(streamEntity));

		dataStart = boundaries->data() + boundaries->size() + 2;
	} while (dataStart < dataEnd);

	SENDLOG(Info, SceneFromMap, "Parsing map took %.2fms\n", (TimeMoment::Now() - start).ToMSec_f64());
}

void ParseObjectIntoEntitiesStream(string_view object, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper, KeyValue &keyValueStorage, EntityObject &entity, bool &isAddedCamera)
{
	entity.id = idGenerator.Generate();

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
			ParseSubobjectIntoEntity(*boundaries, pathToMapAssets, assetIdMapper, entity, isAddedCamera, keyValueStorage);
			const char *boundariesEnd = boundaries->data() + boundaries->size();
			const char *currentEnd = current.data() + current.size();
			current = string_view{boundariesEnd + 2, static_cast<uiw>(currentEnd - boundariesEnd - 2)};
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
			entity.id.DebugName(value);
		}
		else if (key == "position")
		{
			entity.components.AddComponent(Position{.position = ReadVec3(value)});
		}
		else if (key == "rotation")
		{
			entity.components.AddComponent(Rotation{.rotation = ReadVec4<Quaternion>(value)});
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
				entity.components.AddComponent(scale);
			}
		}
		else
		{
			SOFTBREAK;
		}

		current = current.substr(std::min(endOfLine + 1, current.size()));
	}
}

void ParseSubobjectIntoEntity(string_view object, const FilePath &pathToMapAssets, AssetIdMapper &assetIdMapper, EntityObject &entity, bool &isAddedCamera, KeyValue &keyValueStorage)
{
	uiw delim = object.find(':');
	if (delim == string_view::npos)
	{
		SOFTBREAK;
		return;
	}

	string_view subObjectName = object.substr(0, delim);
	string_view subObjectFields = object.substr(std::min(delim + 2, object.size()));

	keyValueStorage.Parse(subObjectFields);

	auto createMeshAsset = [&pathToMapAssets, &assetIdMapper](const KeyValue &keyValueStorage) -> MeshAssetId
	{
		string_view assetPath;
		f32 globalScale = 1.0f;
		optional<ui8> subMeshIndex{};
		bool isUseFileScale = true;
		for (const auto &[key, value] : keyValueStorage.storage)
		{
			if (key == "path")
			{
				assetPath = value;
			}
			else if (key == "meshBaseIndex")
			{
				subMeshIndex = ReadUI32(value);
			}
			else if (key == "globalScale")
			{
				globalScale = ReadF32(value);
			}
			else if (key == "isUseFileScale")
			{
				isUseFileScale = ReadUI32(value) != 0;
			}
		}
		FilePath fullPath = (pathToMapAssets + FilePath::FromChar(assetPath)).GetNormalized();
		if (fullPath.ExtensionView() == TSTR("spm")) // currently ignoring SpeedTree files
		{
			return {};
		}
		auto identification = MeshPathAssetIdentification::New(fullPath, subMeshIndex, globalScale, isUseFileScale);
		MeshAssetId id = assetIdMapper.Register<MeshAsset>(identification);
		id.DebugName(assetPath);
		return id;
	};

	if (subObjectName == "meshrenderer" || subObjectName == "terrain")
	{
		MeshRenderer meshRenderer;
		meshRenderer.mesh = createMeshAsset(keyValueStorage);
		if (meshRenderer.mesh)
		{
			entity.components.AddComponent(meshRenderer);
		}
	}
	else if (subObjectName == "camera")
	{
		Camera camera = CreateDefaultCamera();

		for (const auto &[key, value] : keyValueStorage.storage)
		{
			if (key == "fov")
			{
				camera.fov = ReadF32(value);
			}
			else
			{
				SOFTBREAK;
			}
		}

		entity.components.AddComponent(camera);
		entity.components.AddComponent(ActiveCamera());

		isAddedCamera = true;
	}
	else if (subObjectName == "boxcollider")
	{
		BoxCollider box;
		for (const auto &[key, value] : keyValueStorage.storage)
		{
			if (key == "isTrigger")
			{
				box.isTrigger = ReadUI32(value) != 0;
			}
			else if (key == "center")
			{
				box.center = ReadVec3(value);
			}
			else if (key == "rotation")
			{
				box.rotation = ReadVec4<Quaternion>(value);
			}
			else if (key == "size")
			{
				box.size = ReadVec3(value);
			}
			else
			{
				SOFTBREAK;
			}
		}
		entity.components.AddComponent(box);
	}
	else if (subObjectName == "spherecollider")
	{
		SphereCollider sphere;
		for (const auto &[key, value] : keyValueStorage.storage)
		{
			if (key == "isTrigger")
			{
				sphere.isTrigger = ReadUI32(value) != 0;
			}
			else if (key == "center")
			{
				sphere.center = ReadVec3(value);
			}
			else if (key == "radius")
			{
				sphere.radius = ReadF32(value);
			}
			else
			{
				SOFTBREAK;
			}
		}
		entity.components.AddComponent(sphere);
	}
	else if (subObjectName == "capsulecollider")
	{
		CapsuleCollider capsule;
		for (const auto &[key, value] : keyValueStorage.storage)
		{
			if (key == "isTrigger")
			{
				capsule.isTrigger = ReadUI32(value) != 0;
			}
			else if (key == "center")
			{
				capsule.center = ReadVec3(value);
			}
			else if (key == "radius")
			{
				capsule.radius = ReadF32(value);
			}
			else if (key == "height")
			{
				capsule.height = ReadF32(value);
			}
			else if (key == "direction")
			{
				char direction = ReadChar(value);
				if (direction == 'X')
				{
					capsule.direction = CapsuleCollider::Direction::X;
				}
				else if (direction == 'Y')
				{
					capsule.direction = CapsuleCollider::Direction::Y;
				}
				else if (direction == 'Z')
				{
					capsule.direction = CapsuleCollider::Direction::Z;
				}
				else
				{
					SOFTBREAK;
				}
			}
			else
			{
				SOFTBREAK;
			}
		}
		entity.components.AddComponent(capsule);
	}
	else if (subObjectName == "meshcollider")
	{
		MeshCollider meshCollider;
		for (const auto &[key, value] : keyValueStorage.storage)
		{
			if (key == "isTrigger")
			{
				meshCollider.isTrigger = ReadUI32(value) != 0;
			}
			else if (key == "vertexlimit")
			{
				ui32 decoded = ReadUI32(value);
				if (auto limit = std::numeric_limits<decltype(meshCollider.vertexLimit)>::max(); decoded > limit)
				{
					SOFTBREAK;
					decoded = limit;
				}
				meshCollider.vertexLimit = static_cast<decltype(meshCollider.vertexLimit)>(decoded);
			}
		}
		meshCollider.mesh = createMeshAsset(keyValueStorage);
		if (meshCollider.mesh)
		{
			entity.components.AddComponent(meshCollider);
		}
	}
	else if (subObjectName == "rigidbody")
	{
		PhysicsProperties physics;
		for (const auto &[key, value] : keyValueStorage.storage)
		{
			if (key == "mass")
			{
				physics.mass = ReadF32(value);
			}
			else if (key == "lineardamping")
			{
				physics.linearDamping = ReadF32(value);
			}
			else if (key == "angulardamping")
			{
				physics.angularDamping = ReadF32(value);
			}
			else if (key == "motioncontrol")
			{
				if (value == "kinematic")
				{
					physics.motionControl = PhysicsProperties::MotionControl::Kinematic;
				}
				else if (value == "others_and_gravity")
				{
					physics.motionControl = PhysicsProperties::MotionControl::OthersAndGravity;
				}
				else if (value == "others")
				{
					physics.motionControl = PhysicsProperties::MotionControl::Others;
				}
				else
				{
					SOFTBREAK;
				}
			}
			else if (key == "lockpositionaxis")
			{
				if (value.find('X') != string_view::npos)
				{
					physics.lockPositionAxis.x = true;
				}
				if (value.find('Y') != string_view::npos)
				{
					physics.lockPositionAxis.y = true;
				}
				if (value.find('Z') != string_view::npos)
				{
					physics.lockPositionAxis.z = true;
				}
			}
			else if (key == "lockrotationaxis")
			{
				if (value.find('X') != string_view::npos)
				{
					physics.lockRotationAxis.x = true;
				}
				if (value.find('Y') != string_view::npos)
				{
					physics.lockRotationAxis.y = true;
				}
				if (value.find('Y') != string_view::npos)
				{
					physics.lockRotationAxis.y = true;
				}
			}
			else if (key == "centerofmass")
			{
				physics.centerOfMass = ReadVec3(value);
			}
			else if (key == "intertiatensor")
			{
				physics.inertiaTensor = ReadVec3(value);
			}
			else if (key == "intertiatensorrotation")
			{
				physics.inertiaTensorRotation = ReadVec4<Quaternion>(value);
			}
			else if (key == "solveriterations")
			{
				physics.solverPositionIterations = ReadUI32(value);
			}
			else if (key == "solvervelocityiterations")
			{
				physics.solverVelocityIterations = ReadUI32(value);
			}
			else if (key == "maxangularvelocity")
			{
				physics.maxAngularVelocity = ReadF32(value);
			}
			else if (key == "maxdepenetrationvelocity")
			{
				physics.maxDepenetrationVelocity = ReadF32(value);
			}
			else if (key == "sleepthreshold")
			{
				physics.sleepThreshold = ReadF32(value);
			}
			else
			{
				SOFTBREAK;
			}
		}

		Physics physicsComponent;
		physicsComponent.physics = assetIdMapper.Register<PhysicsPropertiesAsset>(PhysicsPropertiesAssetIdentification::New(move(physics)));
		entity.components.AddComponent(physicsComponent);
	}
	else
	{
		SOFTBREAK;
		return;
	}
}

f32 ReadF32(string_view source)
{
	f32 value;
	if (auto [pointer, error] = std::from_chars(source.data(), source.data() + source.size(), value); error != std::errc())
	{
		SOFTBREAK;
		return {};
	}
	return value;
}

ui32 ReadUI32(string_view source)
{
	ui32 value;
	if (auto [pointer, error] = std::from_chars(source.data(), source.data() + source.size(), value); error != std::errc())
	{
		SOFTBREAK;
		return {};
	}
	return value;
}

char ReadChar(string_view source)
{
	if (source.size() != 1)
	{
		SOFTBREAK;
		return {};
	}
	return source[0];
}

template<typename T> T ReadVec3(string_view source)
{
	T result;

	string_view xStr = source;
	uiw xEnd = xStr.find(',');
	if (xEnd == string_view::npos)
	{
		SOFTBREAK;
		return {};
	}
	result.x = ReadF32(xStr.substr(0, xEnd));

	string_view yStr = xStr.substr(xEnd + 1);
	uiw yEnd = yStr.find(',');
	if (yEnd == string_view::npos)
	{
		SOFTBREAK;
		return {};
	}
	result.y = ReadF32(yStr.substr(0, yEnd));

	string_view zStr = yStr.substr(yEnd + 1);
	result.z = ReadF32(zStr);

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
	result.x = ReadF32(xStr.substr(0, xEnd));

	string_view yStr = xStr.substr(xEnd + 1);
	uiw yEnd = yStr.find(',');
	if (yEnd == string_view::npos)
	{
		SOFTBREAK;
		return {};
	}
	result.y = ReadF32(yStr.substr(0, yEnd));

	string_view zStr = yStr.substr(yEnd + 1);
	uiw zEnd = zStr.find(',');
	if (zEnd == string_view::npos)
	{
		SOFTBREAK;
		return {};
	}
	result.z = ReadF32(zStr.substr(0, zEnd));

	string_view wStr = zStr.substr(zEnd + 1);
	result.w = ReadF32(wStr);

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