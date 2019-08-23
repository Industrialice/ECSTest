#include "PreHeader.hpp"
#include "RendererDX11.hpp"
#include "Scene.hpp"
#include "SceneFromMap.hpp"
#include "CustomControlActions.hpp"
#include "CameraMovementSystem.hpp"
#include "ObjectsMoverSystem.hpp"
#include "PhysicsSystem.hpp"
#include "AssetsLoaders.hpp"

using namespace ECSEngine;

namespace
{
    using LoggerType = Logger<string_view, true>;
    LoggerType::ListenerHandle LoggerVSHandle{}, LoggerFileHandle{};
    File LogFile{};
    KeyController::ListenerHandle InputHandle{};
    std::atomic<bool> IsExiting{false};
}

static void LogRecipient(LogLevels::LogLevel logLevel, StringViewNullTerminated message, string_view senderName);
static void FileLogRecipient(File &file, LogLevels::LogLevel logLevel, StringViewNullTerminated message, string_view senderName);
static bool ReceiveInput(const ControlAction &action);
static TypeId ResolveAssetExtensionToType(const wchar_t *ext);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    StdLib::Initialization::Initialize({});

    EngineLogger = make_shared<LoggerType>();

    LogFile = File(L"log.txt", FileOpenMode::CreateAlways, FileProcModes::Write);
    if (LogFile)
    {
        LogFile.Buffer(16384);
        LoggerFileHandle = EngineLogger->OnMessage(std::bind(FileLogRecipient, std::ref(LogFile), _1, _2, _3));
    }

    LoggerVSHandle = EngineLogger->OnMessage(LogRecipient);

    SENDLOG(Info, WinMain, "Engine has started\n");

    auto manager = SystemsManager::New(false, EngineLogger);

    auto rendererKeyController = KeyController::New();
    InputHandle = rendererKeyController->OnControlAction(ReceiveInput);

    auto renderer = RendererDX11System::New();
    renderer->SetKeyController(rendererKeyController);

	auto cameraMovementSystem = make_unique<CameraMovementSystem>();
	cameraMovementSystem->SetKeyController(KeyController::New()); // create a new key controller for each system because otherwise the same key controller might receive the same control more than once (when the controls are dispatched before system execution)

    auto rendererPipeline = manager->CreatePipeline(nullopt, false);
    manager->Register(move(renderer), rendererPipeline);

	auto physicsPipeline = manager->CreatePipeline(TimeSecondsFP64(1.0 / 60.0), false);
	//manager->Register<ObjectsMoverSystem>(physicsPipeline);
	manager->Register(move(cameraMovementSystem), physicsPipeline);

	PhysicsSystemSettings physicsSystemSettings{};
	manager->Register(PhysicsSystem::New(physicsSystemSettings), physicsPipeline);

	auto assetIdMapper = make_shared<AssetIdMapper>();

	//auto fileEnumerateCallback = [&assetIdMapper](const FileEnumInfo &info, const FilePath &currentPath)
	//{
	//	const wchar_t *ext = wcsrchr(info.cFileName, '.');
	//	if (!ext)
	//	{
	//		return;
	//	}

	//	TypeId type = ResolveAssetExtensionToType(ext + 1);
	//	if (!type.IsValid())
	//	{
	//		SOFTBREAK;
	//		return;
	//	}

	//	assetIdMapper->Register(currentPath.GetWithRemovedTopLevel() / info.cFileName, type);
	//};

	//FileSystem::Enumerate(L"Assets", fileEnumerateCallback, FileSystem::EnumerateOptions::ReportFiles.Combined(FileSystem::EnumerateOptions::Recursive));

	AssetsManager assetsManager;
	AssetsLoaders assetsLoaders;
	assetsLoaders.SetAssetIdMapper(assetIdMapper);
	assetsLoaders.RegisterLoaders(assetsManager);

	std::wstring mapName = L"et";

    vector<WorkerThread> workers;
    EntityIDGenerator idGenerator;
	auto stream = SceneFromMap::Create(L"Assets\\Scenes\\" + mapName + L".imf", L"Assets\\Scenes\\" + mapName + L"_assets\\", idGenerator, *assetIdMapper);
    //auto stream = Scene::Create(idGenerator, *assetIdMapper, assetsManager);

    manager->Start(move(assetsManager), move(idGenerator), move(workers), move(stream));

    for (;;)
    {
        auto info = manager->GetPipelineInfo(rendererPipeline);
        if (IsExiting)
        {
            break;
        }
		std::this_thread::sleep_for(1ms);
    }

    manager->Stop(true);
	manager = {};

    SENDLOG(Info, WinMain, "Engine has finished\n");

    EngineLogger = {};
    LogFile = {};

    return 0;
}

static const char *LogLevelToTag(LogLevels::LogLevel logLevel)
{
    switch (logLevel.AsInteger())
    {
    case LogLevels::Critical.AsInteger():
        return "[crt] ";
    case LogLevels::Debug.AsInteger():
        return "[dbg] ";
    case LogLevels::Error.AsInteger():
        return "[err] ";
    case LogLevels::Attention.AsInteger():
        return "[imp] ";
    case LogLevels::Info.AsInteger():
        return "[inf] ";
    case LogLevels::Other.AsInteger():
        return "[oth] ";
    case LogLevels::Warning.AsInteger():
        return "[wrn] ";
    case LogLevels::_None.AsInteger():
    case LogLevels::_All.AsInteger():
        HARDBREAK;
        return "";
    }

    UNREACHABLE;
    return nullptr;
}

void LogRecipient(LogLevels::LogLevel logLevel, StringViewNullTerminated message, string_view senderName)
{
    if (logLevel == LogLevels::Critical || logLevel == LogLevels::Debug || logLevel == LogLevels::Error)
    {
        SOFTBREAK;
    }

    if (logLevel == LogLevels::Critical/* || logLevel == LogLevel::Debug || logLevel == LogLevel::Error*/)
    {
        const char *tag = nullptr;
        switch (logLevel.AsInteger()) // fill out all the cases just in case
        {
        case LogLevels::Critical.AsInteger():
            tag = "CRITICAL";
            break;
        case LogLevels::Debug.AsInteger():
            tag = "DEBUG";
            break;
        case LogLevels::Error.AsInteger():
            tag = "ERROR";
            break;
        case LogLevels::Attention.AsInteger():
            tag = "IMPORTANT INFO";
            break;
        case LogLevels::Info.AsInteger():
            tag = "INFO";
            break;
        case LogLevels::Other.AsInteger():
            tag = "OTHER";
            break;
        case LogLevels::Warning.AsInteger():
            tag = "WARNING";
            break;
        case LogLevels::_None.AsInteger():
        case LogLevels::_All.AsInteger():
            HARDBREAK;
            return;
        }

        MessageBoxA(0, message.data(), tag, 0);
        return;
    }

    const char *tag = LogLevelToTag(logLevel);

    OutputDebugStringA(tag);
    OutputDebugStringA(senderName.data());
    OutputDebugStringA(": ");
    OutputDebugStringA(message.data());

    printf("%s%s: %s", tag, senderName.data(), message.data());
}

void FileLogRecipient(File &file, LogLevels::LogLevel logLevel, StringViewNullTerminated message, string_view senderName)
{
    const char *tag = LogLevelToTag(logLevel);
    file.Write(tag, static_cast<ui32>(strlen(tag)));
    file.Write(senderName.data(), static_cast<ui32>(senderName.size()));
    file.Write(": ", 2);
    file.Write(message.data(), static_cast<ui32>(message.size()));
}

bool ReceiveInput(const ControlAction &action)
{
    if (auto custom = action.Get<ControlAction::Custom>(); custom)
    {
        if (custom->type == CustomControlAction::WindowClosed::GetTypeId())
        {
            SENDLOG(Info, ReceiveInput, "Received WindowClosed event, stopping application\n");
            IsExiting = true;
        }
    }
    else if (auto key = action.Get<ControlAction::Key>(); key)
    {
        if (key->key == KeyCode::Escape)
        {
            SENDLOG(Info, ReceiveInput, "Received escape key, stopping application\n");
            IsExiting = true;
        }
    }

    return false;
}

TypeId ResolveAssetExtensionToType(const wchar_t *ext)
{
	static constexpr wchar_t meshExtensions[][8] =
	{
		L"fbx", L"dae", L"gltf", L"glb", L"blend", L"3ds", L"ase", L"obj", L"ifc", L"xgl", L"zgl", L"ply", L"dxf", L"lwo", L"lws", L"lxo",
		L"stl", L"x", L"ac", L"ms3d", L"cob", L"scn", L"bvh", L"csm", L"xml", L"irrmesh", L"irr", L"mdl", L"md2", L"md3", L"pk3", L"mdc",
		L"md5", L"smd", L"vta", L"ogex", L"3d", L"b3d", L"q3d", L"q3s", L"nff", L"nff", L"off", L"raw", L"ter", L"mdl", L"hmp", L"ndo"
	};

	for (const wchar_t *meshExt : meshExtensions)
	{
		if (!_wcsicmp(ext, meshExt))
		{
			return MeshAsset::GetTypeId();
		}
	}

	return {};
}