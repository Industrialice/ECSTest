#include "PreHeader.hpp"
#include "RendererDX11.hpp"
#include "Scene.hpp"
#include "CustomControlActions.hpp"

using namespace ECSEngine;

namespace
{
    using LoggerType = Logger<string_view, true>;
    shared_ptr<LoggerType> EngineLogger = make_shared<LoggerType>();
    LoggerType::ListenerHandle LoggerVSHandle{}, LoggerFileHandle{};
    File LogFile{};
    KeyController::ListenerHandle InputHandle{};
    std::atomic<bool> IsExiting{false};
}

static void LogRecipient(LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName);
static void FileLogRecipient(File &file, LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName);
static bool ReceiveInput(const ControlAction &action);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    StdLib::Initialization::Initialize({});

    LogFile = File(L"log.txt", FileOpenMode::CreateAlways, FileProcModes::Write);
    if (LogFile)
    {
        LoggerFileHandle = EngineLogger->OnMessage(std::bind(FileLogRecipient, std::ref(LogFile), _1, _2, _3));
    }

    LoggerVSHandle = EngineLogger->OnMessage(LogRecipient);

    EngineLogger->Message(LogLevels::Info, "WinMain", "Engine has started\n");

    auto manager = SystemsManager::New(false, EngineLogger);

    auto keyController = KeyController::New();
    InputHandle = keyController->OnControlAction(ReceiveInput);

    auto renderer = RendererDX11System::New();
    renderer->SetKeyController(keyController);

    auto rendererPipeline = manager->CreatePipeline(nullopt, false);
    manager->Register(move(renderer), rendererPipeline);

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
        std::this_thread::yield();
    }

    manager->Stop(true);

    EngineLogger->Message(LogLevels::Info, "WinMain", "Engine has finished\n");

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
    if (logLevel == LogLevels::Critical || logLevel == LogLevels::Debug || logLevel == LogLevels::Error) // TODO: cancel breaking
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
            IsExiting = true;
        }
    }

    return false;
}