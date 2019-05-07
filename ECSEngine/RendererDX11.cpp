#include "PreHeader.hpp"
#include "RendererDX11.hpp"
#include "Components.hpp"

using namespace ECSEngine;

static optional<HWND> CreateSystemWindow(LoggerWrapper &logger, const string &title, bool isFullscreen, bool hideBorders, bool isMaximized, RECT &dimensions, Window::CursorTypet cursorType, void *userData);
static LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HCURSOR AcquireCursor(Window::CursorTypet type);

struct DX11Camera
{
    EntityID id{};
    Camera data{};
    optional<HWND> hwnd{};
    bool isChanged = false;

    ~DX11Camera()
    {
        if (hwnd)
        {
            DestroyWindow(*hwnd);
        }
    }
};

class RendereDX11SystemImpl : public RendererDX11System
{
public:
    virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
    {
        for (auto &entry : stream)
        {
            auto camera = entry.FindComponent<Camera>();
            if (camera)
            {
                AddCamera(entry.entityID, *camera);
            }
        }
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
    {
        if (stream.Type() == Camera::GetTypeId())
        {
            for (auto &entry : stream)
            {
                AddCamera(entry.entityID, entry.added.Cast<Camera>());
            }
        }
        else
        {
            HARDBREAK;
        }
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
    {
        if (stream.Type() == Camera::GetTypeId())
        {
            for (auto &entry : stream)
            {
                CameraChanged(entry.entityID, entry.component.Cast<Camera>());
            }
        }
        else if (stream.Type() == Position::GetTypeId())
        {
        }
        else if (stream.Type() == Rotation::GetTypeId())
        {
        }
        else
        {
            HARDBREAK;
        }
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
    {
        if (stream.Type() == Camera::GetTypeId())
        {
            for (auto &entry : stream)
            {
                RemoveCamera(entry.entityID);
            }
        }
        else if (stream.Type() == Position::GetTypeId())
        {
        }
        else if (stream.Type() == Rotation::GetTypeId())
        {
        }
        else
        {
            HARDBREAK;
        }
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
    {
        for (auto id : stream)
        {
            RemoveCamera(id);
        }
    }
    
    virtual void Update(Environment &env) override
    {
        for (auto &camera : _cameras)
        {
            if (!camera.hwnd)
            {
                string title = "RendererDX11: ";
                auto window = std::get<Window>(camera.data.rts[0].target);
                title += string_view(window.title.data(), window.title.size());

                RECT dim;
                dim.left = window.x;
                dim.right = window.x + window.width;
                dim.top = window.y;
                dim.bottom = window.y + window.height;

                camera.hwnd = CreateSystemWindow(env.logger, title, window.isFullscreen, window.isNoBorders, window.isMaximized, dim, window.cursorType, &camera);
            }
        }

        MSG msg{};
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (msg.message == WM_QUIT)
        {
        }

        for (auto &camera : _cameras)
        {
            if (camera.isChanged)
            {
                env.messageBuilder.ComponentChanged(camera.id, camera.data);
                camera.isChanged = false;
            }
        }
    }

private:
    void AddCamera(EntityID id, const Camera &data)
    {
        _cameras.push_front({id, data});
    }

    void RemoveCamera(EntityID id)
    {
        _cameras.remove_if([id](const DX11Camera &stored) { return stored.id == id; });
    }

    // TODO: handle all parameters
    void CameraChanged(EntityID id, const Camera &data)
    {
        for (auto &camera : _cameras)
        {
            if (camera.id == id)
            {
                if (camera.isChanged)
                {
                    SOFTBREAK;
                    camera.isChanged = false;
                }

                auto &current = std::get<Window>(camera.data.rts[0].target);
                const auto &source = std::get<Window>(data.rts[0].target);

                if (camera.hwnd)
                {
                    if (current.cursorType != source.cursorType)
                    {
                        SetCursor(AcquireCursor(source.cursorType));
                    }
                    if (current.title != source.title)
                    {
                        SetWindowTextA(*camera.hwnd, source.title.data());
                    }
                    if (current.x != source.x || current.y != source.y || current.width != source.width || current.height != source.height)
                    {
                        BOOL result = SetWindowPos(*camera.hwnd, NULL, source.x, source.y, source.width, source.height, SWP_NOSIZE | SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOCOPYBITS | SWP_NOACTIVATE);
                        ASSUME(result == TRUE);
                    }
                }

                current = source;

                return;
            }
        }

        UNREACHABLE;
    }

private:
    std::forward_list<DX11Camera> _cameras{};
};

unique_ptr<Renderer> RendererDX11System::New()
{
    return make_unique<RendereDX11SystemImpl>();
}

optional<HWND> CreateSystemWindow(LoggerWrapper &logger, const string &title, bool isFullscreen, bool hideBorders, bool isMaximized, RECT &dimensions, Window::CursorTypet cursorType, void *userData)
{
    HINSTANCE instance = GetModuleHandleW(NULL);
    LARGE_INTEGER seed;
    QueryPerformanceCounter(&seed);
    auto className = title + to_string(seed.QuadPart);

    WNDCLASSA wc;
    wc.style = CS_HREDRAW | CS_VREDRAW; // means that we need to redraw the whole window if its size changes, not just a new portion
    wc.lpfnWndProc = MsgProc; // note that the message procedure runs in the same thread that created the window (it's a requirement)
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIconA(0, IDI_APPLICATION);
    wc.hCursor = AcquireCursor(cursorType);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = className.c_str();

    if (!RegisterClassA(&wc))
    {
        logger.Message(LogLevels::Critical, "Failed to register class\n");
        return nullopt;
    }

    if (isFullscreen)
    {
        ShowCursor(FALSE);
    }

    DWORD style = isFullscreen || hideBorders ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    if (isMaximized)
    {
        style |= WS_MAXIMIZE;
    }

    AdjustWindowRect(&dimensions, style, !(hideBorders || isFullscreen));

    i32 x = dimensions.left;
    i32 y = dimensions.top;
    i32 width = dimensions.right - dimensions.left;
    i32 height = dimensions.bottom - dimensions.top;

    HWND hwnd = CreateWindowA(className.c_str(), title.c_str(), style, x, y, width, height, 0, 0, instance, 0);
    if (!hwnd)
    {
        logger.Message(LogLevels::Critical, "Failed to create requested window\n");
        return nullopt;
    }

    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)userData);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto data = (DX11Camera *)GetWindowLongA(hwnd, GWLP_USERDATA);
    if (data == nullptr || data->hwnd != hwnd)
    {
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    switch (msg)
    {
    case WM_INPUT:
        // handle HID input
        break;
    case WM_SETCURSOR:
    case WM_MOUSEMOVE:
    case WM_KEYDOWN:
    case WM_KEYUP:
        // handle VK input
        break;
    case WM_ACTIVATE:
        //  switching window's active state
        break;
    case WM_SIZE:
        {
            auto &window = std::get<Window>(data->data.rts[0].target);
            window.width = LOWORD(lParam);
            window.height = HIWORD(lParam);
            data->isChanged = true;
        } break;
        // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
        break;
        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
    case WM_EXITSIZEMOVE:
        break;
        // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        // somebody has asked our window to close
    case WM_CLOSE:
        break;
        // Catch this message so to prevent the window from becoming too small.
    case WM_GETMINMAXINFO:
        //((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        //((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
        break;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

HCURSOR AcquireCursor(Window::CursorTypet type)
{
    using CursorType = Window::CursorTypet;

    switch (type)
    {
    case CursorType::Normal:
        return LoadCursorA(NULL, IDC_ARROW);
    case CursorType::Busy:
        return LoadCursorA(NULL, IDC_WAIT);
    case CursorType::Hand:
        return LoadCursorA(NULL, IDC_HAND);
    case CursorType::Target:
        return LoadCursorA(NULL, IDC_CROSS);
    case CursorType::No:
        return LoadCursorA(NULL, IDC_NO);
    case CursorType::Help:
        return LoadCursorA(NULL, IDC_HELP);
    }

    UNREACHABLE;
    return NULL;
}