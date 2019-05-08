#include "PreHeader.hpp"
#include "RendererDX11.hpp"
#include "Scene.hpp"
#include "CustomControlActions.hpp"

using namespace ECSEngine;

namespace
{
    using LoggerType = Logger<string_view, true>;
    LoggerType::ListenerHandle LoggerVSHandle{}, LoggerFileHandle{};
    File LogFile{};
    KeyController::ListenerHandle InputHandle{};
    std::atomic<bool> IsExiting{false};
}

static void LogRecipient(LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName);
static void FileLogRecipient(File &file, LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName);
static bool ReceiveInput(const ControlAction &action);

DIRECT_SYSTEM(ScreenColorSystem)
{
	DIRECT_ACCEPT_COMPONENTS(Array<Camera> &cameras, const Array<EntityID> &ids)
	{
		for (auto &camera : cameras)
		{
			ClearColor clearColor;
			clearColor.color = ColorR8G8B8((ui32)intensity, (ui32)intensity, (ui32)intensity);
			camera.clearWith = clearColor;
		}
	}

	virtual bool ControlInput(Environment &env, const ControlAction &action) override
	{
		if (auto wheel = action.Get<ControlAction::MouseWheel>(); wheel)
		{
			intensity -= wheel->delta * 10;
			intensity = std::clamp(intensity, 0, 255);
		}

		return false;
	}

private:
	i32 intensity = 0;
};

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    StdLib::Initialization::Initialize({});

    EngineLogger = make_shared<LoggerType>();

    LogFile = File(L"log.txt", FileOpenMode::CreateAlways, FileProcModes::Write);
    if (LogFile)
    {
        LogFile.BufferSet(16384);
        LoggerFileHandle = EngineLogger->OnMessage(std::bind(FileLogRecipient, std::ref(LogFile), _1, _2, _3));
    }

    LoggerVSHandle = EngineLogger->OnMessage(LogRecipient);

    SENDLOG(Info, WinMain, "Engine has started\n");

    auto manager = SystemsManager::New(false, EngineLogger);

    auto keyController = KeyController::New();
    InputHandle = keyController->OnControlAction(ReceiveInput);

    auto renderer = RendererDX11System::New();
    renderer->SetKeyController(keyController);

	auto screenColorSystem = make_unique<ScreenColorSystem>();
	screenColorSystem->SetKeyController(KeyController::New());

    auto rendererPipeline = manager->CreatePipeline(nullopt, false);
    manager->Register(move(renderer), rendererPipeline);
	manager->Register(move(screenColorSystem), rendererPipeline);

    vector<WorkerThread> workers;
    EntityIDGenerator idGenerator;
    auto stream = Scene::Create(idGenerator);

    manager->Start(move(idGenerator), move(workers), move(stream));

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

void LogRecipient(LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName)
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

        MessageBoxA(0, nullTerminatedText.data(), tag, 0);
        return;
    }

    const char *tag = LogLevelToTag(logLevel);

    OutputDebugStringA(tag);
    OutputDebugStringA(senderName.data());
    OutputDebugStringA(": ");
    OutputDebugStringA(nullTerminatedText.data());

    printf("%s%s: %s", tag, senderName.data(), nullTerminatedText.data());
}

void FileLogRecipient(File &file, LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName)
{
    const char *tag = LogLevelToTag(logLevel);
    file.Write(tag, (ui32)strlen(tag));
    file.Write(senderName.data(), (ui32)senderName.size());
    file.Write(": ", 2);
    file.Write(nullTerminatedText.data(), (ui32)nullTerminatedText.size());
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