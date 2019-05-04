#include "PreHeader.hpp"

namespace
{
    using LoggerType = Logger<string_view, true>;
    shared_ptr<LoggerType> EngineLogger = make_shared<LoggerType>();
    LoggerType::ListenerHandle LoggerVSHandle{}, LoggerFileHandle{};
    File LogFile{};
}

static optional<HWND> CreateSystemWindow(bool isFullscreen, const string &title, HINSTANCE instance, bool hideBorders, RECT &dimensions);
static LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void MessageLoop();
static void LogRecipient(LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName);
static void FileLogRecipient(File &file, LogLevels::LogLevel logLevel, string_view nullTerminatedText, string_view senderName);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    StdLib::Initialization::Initialize({});

    LogFile = File(L"log.txt", FileOpenMode::CreateAlways, FileProcModes::Write);
    if (LogFile)
    {
        LoggerFileHandle = EngineLogger->OnMessage(std::bind(FileLogRecipient, LogFile, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    LoggerVSHandle = EngineLogger->OnMessage(LogRecipient);

    EngineLogger->Message(LogLevels::Info, "WinMain", "Engine has started\n");

    auto manager = SystemsManager::New(false, EngineLogger);

    EngineLogger->Message(LogLevels::Info, "WinMain", "Engine has finished\n");

    return 0;
}

//optional<HWND> CreateSystemWindow(bool isFullscreen, const string &title, HINSTANCE instance, bool hideBorders, RECT &dimensions)
//{
//    auto className = "window_" + title;
//
//    WNDCLASSA wc;
//    wc.style = CS_HREDRAW | CS_VREDRAW; // means that we need to redraw the whole window if its size changes, not just a new portion
//    wc.lpfnWndProc = MsgProc; // note that the message procedure runs in the same thread that created the window (it's a requirement)
//    wc.cbClsExtra = 0;
//    wc.cbWndExtra = 0;
//    wc.hInstance = instance;
//    wc.hIcon = LoadIconA(0, IDI_APPLICATION);
//    wc.hCursor = LoadCursorA(0, IDC_ARROW);
//    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
//    wc.lpszMenuName = 0;
//    wc.lpszClassName = className.c_str();
//
//    if (!RegisterClassA(&wc))
//    {
//        EngineLogger->Message(LogLevels::Critical, "CreateSystemWindow", "Failed to register class\n");
//        return nullopt;
//    }
//
//    ui32 width = dimensions.right - dimensions.left;
//    ui32 height = dimensions.bottom - dimensions.top;
//
//    if (isFullscreen)
//    {
//        ShowCursor(FALSE);
//    }
//
//    DWORD style = isFullscreen || hideBorders ? WS_POPUP : WS_SYSMENU;
//
//    AdjustWindowRect(&dimensions, style, false);
//
//    HWND hwnd = CreateWindowA(className.c_str(), title.c_str(), style, dimensions.left, dimensions.top, width, height, 0, 0, instance, 0);
//    if (!hwnd)
//    {
//        EngineLogger->Message(LogLevels::Critical, "CreateSystemWindow", "Failed to create requested window\n");
//        return nullopt;
//    }
//
//    ShowWindow(hwnd, SW_SHOW);
//    UpdateWindow(hwnd);
//
//    return hwnd;
//}
//
//LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//    WindowData *windowData = nullptr;
//    if (hwnd == Application::GetMainWindow().hwnd)
//    {
//        windowData = (WindowData *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);;
//        if (windowData == nullptr || windowData->hwnd != hwnd)
//        {
//            SOFTBREAK;
//            windowData = nullptr;
//        }
//    }
//
//    switch (msg)
//    {
//    case WM_INPUT:
//        if (windowData != nullptr && Application::GetHIDInput() != nullptr)
//        {
//            Application::GetHIDInput()->Dispatch(windowData->controlsQueue, hwnd, wParam, lParam);
//            return 0;
//        }
//        break;
//    case WM_SETCURSOR:
//    case WM_MOUSEMOVE:
//    case WM_KEYDOWN:
//    case WM_KEYUP:
//        if (windowData != nullptr && Application::GetVKInput() != nullptr)
//        {
//            Application::GetVKInput()->Dispatch(windowData->controlsQueue, hwnd, msg, wParam, lParam);
//            return 0;
//        }
//        break;
//    case WM_ACTIVATE:
//        //  switching window's active state
//        break;
//    case WM_SIZE:
//        {
//            //window->width = LOWORD( lParam );
//            //window->height = HIWORD( lParam );
//        } break;
//        // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
//    case WM_ENTERSIZEMOVE:
//        break;
//        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
//    case WM_EXITSIZEMOVE:
//        break;
//        // WM_DESTROY is sent when the window is being destroyed.
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        break;
//        // somebody has asked our window to close
//    case WM_CLOSE:
//        break;
//        // Catch this message so to prevent the window from becoming too small.
//    case WM_GETMINMAXINFO:
//        //((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
//        //((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
//        break;
//    }
//
//    return DefWindowProcA(hwnd, msg, wParam, lParam);
//}
//
//void MessageLoop()
//{
//    MSG o_msg;
//    auto lastUpdate = TimeMoment::Now();
//    auto firstUpdate = TimeMoment::Now();
//
//    do
//    {
//        if (PeekMessageA(&o_msg, 0, 0, 0, PM_REMOVE))
//        {
//            TranslateMessage(&o_msg);
//            DispatchMessageA(&o_msg);
//        }
//        else
//        {
//            WindowData *windowData = (WindowData *)GetWindowLongPtrA(Application::GetMainWindow().hwnd, GWLP_USERDATA);
//            if (windowData != nullptr)
//            {
//                Application::GetKeyController().Dispatch(windowData->controlsQueue.Enumerate());
//                windowData->controlsQueue.clear();
//            }
//            else
//            {
//                HARDBREAK;
//            }
//
//            auto currentMemont = TimeMoment::Now();
//
//            TimeDifference64 durationSinceStart = (currentMemont - firstUpdate).As64();
//            TimeDifference durationSinceLastUpdate = currentMemont - lastUpdate;
//
//            EngineTime engineTime = Application::GetEngineTime();
//            engineTime.secondsSinceStart = durationSinceStart.ToSeconds() * engineTime.timeScale;
//            engineTime.secondSinceLastFrame = durationSinceLastUpdate.ToSeconds() * engineTime.timeScale;
//            engineTime.unscaledSecondSinceLastFrame = durationSinceLastUpdate.ToSeconds();
//
//            lastUpdate = currentMemont;
//
//            Application::SetEngineTime(engineTime);
//
//            Application::GetRenderer().BeginFrame();
//
//            const auto &camera = Application::GetMainCamera();
//            camera->ClearColorRGBA(array<f32, 4>{0, 0, 0, 1});
//            camera->ClearDepthValue(1.0f);
//
//            const auto &keyController = Application::GetKeyController();
//
//            float camMovementScale = engineTime.unscaledSecondSinceLastFrame * 5;
//            if (keyController.GetKeyInfo(KeyCode::LShift).IsPressed())
//            {
//                camMovementScale *= 3;
//            }
//            if (keyController.GetKeyInfo(KeyCode::LControl).IsPressed())
//            {
//                camMovementScale *= 0.33f;
//            }
//
//            if (keyController.GetKeyInfo(KeyCode::S).IsPressed())
//            {
//                camera->MoveAlongForwardAxis(-camMovementScale);
//            }
//            if (keyController.GetKeyInfo(KeyCode::W).IsPressed())
//            {
//                camera->MoveAlongForwardAxis(camMovementScale);
//            }
//            if (keyController.GetKeyInfo(KeyCode::A).IsPressed())
//            {
//                camera->MoveAlongRightAxis(-camMovementScale);
//            }
//            if (keyController.GetKeyInfo(KeyCode::D).IsPressed())
//            {
//                camera->MoveAlongRightAxis(camMovementScale);
//            }
//            if (keyController.GetKeyInfo(KeyCode::R).IsPressed())
//            {
//                camera->MoveAlongUpAxis(camMovementScale);
//            }
//            if (keyController.GetKeyInfo(KeyCode::F).IsPressed())
//            {
//                camera->MoveAlongUpAxis(-camMovementScale);
//            }
//
//            if (keyController.GetKeyInfo(KeyCode::LAlt).IsPressed())
//            {
//                while (UpTimeDeltaCounter != keyController.GetKeyInfo(KeyCode::UpArrow).timesKeyStateChanged)
//                {
//                    SENDLOG(Info, "SENT\n");
//                    ++UpTimeDeltaCounter;
//                    engineTime.timeScale += 0.05f;
//                    Application::SetEngineTime(engineTime);
//                }
//                while (DownTimeDeltaCounter != keyController.GetKeyInfo(KeyCode::DownArrow).timesKeyStateChanged)
//                {
//                    SENDLOG(Info, "SENT\n");
//                    ++DownTimeDeltaCounter;
//                    engineTime.timeScale -= 0.05f;
//                    engineTime.timeScale = std::max(engineTime.timeScale, 0.1f);
//                    Application::SetEngineTime(engineTime);
//                }
//            }
//
//            if (ui32 newCounter = keyController.GetKeyInfo(KeyCode::Space).timesKeyStateChanged; newCounter != SceneRestartedCounter)
//            {
//                SceneRestartedCounter = newCounter;
//                SceneToDraw::Restart();
//            }
//
//            Application::GetRenderer().ClearCameraTargets(camera.get());
//
//            SceneToDraw::Update();
//            SceneToDraw::Draw(*camera);
//
//            Application::GetRenderer().SwapBuffers();
//
//            Application::GetRenderer().EndFrame();
//        }
//    } while (o_msg.message != WM_QUIT);
//}

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