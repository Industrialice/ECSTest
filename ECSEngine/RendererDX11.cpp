#include "PreHeader.hpp"
#include "RendererDX11.hpp"
#include "Components.hpp"
#include "CustomControlActions.hpp"
#include "WinHIDInput.hpp"
#include "WinVKInput.hpp"

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

using namespace ECSEngine;

static optional<HWND> CreateSystemWindow(LoggerWrapper &logger, const string &title, bool isFullscreen, bool hideBorders, bool isMaximized, RECT &dimensions, Window::CursorTypet cursorType, void *userData);
static LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HCURSOR AcquireCursor(Window::CursorTypet type);
static const char *ConvertDX11ErrToString(HRESULT hresult);

struct _COMDeleter
{
    void operator()(IUnknown *ptr)
    {
        if (ptr)
        {
            ptr->Release();
        }
    }
};

template <typename T> using COMUniquePtr = unique_ptr<T, _COMDeleter>;

struct DX11Window
{
    bool isChanged = false;
    Window *window{};
    optional<HWND> hwnd{};
    ControlsQueue controlsQueue{};
    optional<HIDInput> hidInput{};
    optional<VKInput> vkInput{};
    COMUniquePtr<IDXGISwapChain> swapChain{};
    COMUniquePtr<ID3D11RenderTargetView> renderTargetView{};
    i32 currentWidth{}, currentHeight{};
    bool currentFullScreen{};
    ID3D11Device *device{};

private:
    bool _isWaitingForWindowResizeNotification = false;

public:
    ~DX11Window()
    {
        renderTargetView = {};

        if (swapChain)
        {
            swapChain->SetFullscreenState(FALSE, nullptr); // can't call Release while in full-screen
            swapChain = {};
        }

        if (hwnd)
        {
            DestroyWindow(*hwnd);
            hwnd = {};
        }
    }

    DX11Window() = default;

    DX11Window(DX11Window &&source);
    DX11Window &operator = (DX11Window &&source);
    DX11Window(Window &window, ID3D11Device *device, LoggerWrapper &logger);
    bool MakeWindowAssociation(LoggerWrapper &logger);
    bool CreateWindowRenderTargetView(LoggerWrapper &logger);
    bool NotifyWindowResized(LoggerWrapper &logger);
    bool HandleSizeFullScreenChanges(LoggerWrapper &logger);
};

struct DX11Camera
{
    EntityID id{};
    Camera data{};
    DX11Window windows[8]{};

    DX11Camera(EntityID id, const Camera &data) : id(id), data(data) {}
    DX11Camera(DX11Camera &&) = default;
    DX11Camera &operator = (DX11Camera &&) = default;
};

class RendereDX11SystemImpl : public RendererDX11System
{
public:
    virtual bool ControlInput(Environment &env, const ControlAction &input) override
    {
        return false;
    }

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
            for (uiw windowIndex = 0; windowIndex < CountOf(camera.data.rt); ++windowIndex)
            {
                if (auto *window = std::get_if<Window>(&camera.data.rt[windowIndex].target); window)
                {
					auto &dxWindow = camera.windows[windowIndex];
                    if (!dxWindow.hwnd)
                    {
                        new (&dxWindow) DX11Window(*window, _device.get(), env.logger);
                    }
					dxWindow.MakeWindowAssociation(env.logger);
					if (dxWindow.swapChain)
					{
						if (auto *clearColor = std::get_if<ClearColor>(&camera.data.clearWith); clearColor)
						{
							auto color = clearColor->color.ConvertToHDR();
							_context->ClearRenderTargetView(dxWindow.renderTargetView.get(), color.data());
						}
						else
						{
						}

						dxWindow.swapChain->Present(0, 0);
					}
                }
            }


        }

        MSG msg{};
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        for (auto &camera : _cameras)
        {
            bool isChanged = false;

            for (uiw windowIndex = 0; windowIndex < CountOf(camera.data.rt); ++windowIndex)
            {
                isChanged |= camera.windows[windowIndex].isChanged;
                camera.windows[windowIndex].isChanged = false;
                env.keyController->Dispatch(camera.windows[windowIndex].controlsQueue.Enumerate());
                camera.windows[windowIndex].controlsQueue.clear();
            }

            if (isChanged)
            {
                env.messageBuilder.ComponentChanged(camera.id, camera.data);
                isChanged = false;
            }
        }

        if (msg.message == WM_QUIT)
        {
            auto event = make_shared <CustomControlAction::WindowClosed>();
            ControlAction::Custom custom;
            custom.type = event->GetTypeId();
            custom.data = move(event);
            ControlAction action(custom, {}, {});
            env.keyController->Dispatch(action);
        }
    }

    virtual void OnCreate(Environment &env) override
    {
        env.logger.Message(LogLevels::Info, "Initializing\n");

        constexpr ui32 adaptersMask = ui32_max;
        constexpr D3D_FEATURE_LEVEL maxFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        COMUniquePtr<IDXGIFactory1> dxgiFactory;
        HRESULT dxgiFactoryResult = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&dxgiFactory);
        if (FAILED(dxgiFactoryResult))
        {
            env.logger.Message(LogLevels::Error, "Failed to create DXGI factory, error %s\n", ConvertDX11ErrToString(dxgiFactoryResult));
            return;
        }

        ASSUME(dxgiFactory != nullptr);

        vector<COMUniquePtr<IDXGIAdapter>> adapters;
        for (ui32 index = 0; index < 32; ++index)
        {
            if (((1 << index) & adaptersMask) == 0)
            {
                continue;
            }

            IDXGIAdapter *adapter;
            if (dxgiFactory->EnumAdapters(index, &adapter) == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }

            adapters.emplace_back(adapter);
        }

        if (adapters.empty())
        {
            env.logger.Message(LogLevels::Error, "Adapters list is empty, try another adaptersMask\n");
            return;
        }

        UINT createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

    #if defined(DEBUG)
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

        vector<D3D_FEATURE_LEVEL> featureLevels{D3D_FEATURE_LEVEL_9_1, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0};
        for (uiw index = 1; index < featureLevels.size(); ++index)
        {
            if (featureLevels[index] > maxFeatureLevel)
            {
                featureLevels.resize(index);
                break;
            }
        }

        HRESULT result = D3D11CreateDevice(
            adapters[0].get(),
            D3D_DRIVER_TYPE_UNKNOWN,
            NULL,
            createDeviceFlags,
            featureLevels.data(),
            (UINT)featureLevels.size(),
            D3D11_SDK_VERSION,
            (ID3D11Device **)&_device,
            &_featureLevel,
            (ID3D11DeviceContext **)&_context);

        if (FAILED(result))
        {
            env.logger.Message(LogLevels::Error, "Failed to create device, error %s\n", ConvertDX11ErrToString(result));
            return;
        }

        _maxSupportedFeatureLevel = _device->GetFeatureLevel(); // TODO: this can be off

        ASSUME(_device != nullptr && _context != nullptr);
        
        env.logger.Message(LogLevels::Info, "Finished initialization\n");
    }

    virtual void OnDestroy(Environment &env) override
    {
        env.logger.Message(LogLevels::Info, "Deinitializing\n");

        _context = {};
        _device = {};

        env.logger.Message(LogLevels::Info, "Finished deinitialization\n");
    }

private:
    void AddCamera(EntityID id, const Camera &data)
    {
        _cameras.emplace_front(id, data);
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
                for (uiw windowIndex = 0; windowIndex < CountOf(camera.data.rt); ++windowIndex)
                {
                    auto &dxWindow = camera.windows[windowIndex];

                    if (dxWindow.isChanged)
                    {
                        SOFTBREAK;
                        dxWindow.isChanged = false;
                    }

                    auto *current = std::get_if<Window>(&camera.data.rt[windowIndex].target);
                    const auto *source = std::get_if<Window>(&data.rt[windowIndex].target);

					if (current == nullptr && source == nullptr)
					{
						continue;
					}

					if (source == nullptr)
					{
						dxWindow = {};
					}

                    if (dxWindow.hwnd)
                    {
                        if (current->cursorType != source->cursorType)
                        {
                            SetCursor(AcquireCursor(source->cursorType));
                        }
                        if (current->title != source->title)
                        {
                            SetWindowTextA(*dxWindow.hwnd, source->title.data());
                        }
                        if (current->x != source->x || current->y != source->y || current->width != source->width || current->height != source->height)
                        {
                            BOOL result = SetWindowPos(*dxWindow.hwnd, NULL, source->x, source->y, source->width, source->height, SWP_NOSIZE | SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOCOPYBITS | SWP_NOACTIVATE);
                            ASSUME(result == TRUE);
                        }
                    }
                }

                camera.data = data;

                return;
            }
        }

        UNREACHABLE;
    }

private:
    std::forward_list<DX11Camera> _cameras{};
    COMUniquePtr<ID3D11Device> _device{};
    COMUniquePtr<ID3D11DeviceContext> _context{};
    D3D_FEATURE_LEVEL _featureLevel{}, _maxSupportedFeatureLevel{};
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
    auto data = (DX11Window *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    if (data == nullptr || data->hwnd != hwnd)
    {
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    switch (msg)
    {
    case WM_INPUT:
        if (data->hidInput)
        {
            data->hidInput->Dispatch(data->controlsQueue, hwnd, wParam, lParam);
        }
        break;
    case WM_SETCURSOR:
    case WM_MOUSEMOVE:
    case WM_KEYDOWN:
    case WM_KEYUP:
        if (data->vkInput)
        {
            data->vkInput->Dispatch(data->controlsQueue, hwnd, msg, wParam, lParam);
        }
        break;
    case WM_ACTIVATE:
        //  switching window's active state
        break;
    case WM_SIZE:
        {
            auto &window = *data->window;
            window.width = LOWORD(lParam);
            window.height = HIWORD(lParam);
            data->isChanged = true;
			LoggerWrapper logger;
			data->NotifyWindowResized(logger);
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

const char *ConvertDX11ErrToString(HRESULT hresult)
{
    switch (hresult)
    {
    case D3D11_ERROR_FILE_NOT_FOUND:
        return "D3D11_ERROR_FILE_NOT_FOUND";
    case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
        return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
    case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
        return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
    case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
        return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
    case DXGI_ERROR_INVALID_CALL:
        return "DXGI_ERROR_INVALID_CALL";
    case DXGI_ERROR_WAS_STILL_DRAWING:
        return "DXGI_ERROR_WAS_STILL_DRAWING";
    case E_FAIL:
        return "E_FAIL";
    case E_INVALIDARG:
        return "E_INVALIDARG";
    case E_OUTOFMEMORY:
        return "E_OUTOFMEMORY";
    case S_FALSE:
        return "S_FALSE";
    default:
        return "unidentified error";
    }
}

DX11Window::DX11Window(DX11Window &&source) : 
    isChanged(source.isChanged),
    window(source.window),
    hwnd(move(source.hwnd)),
    controlsQueue(move(source.controlsQueue)),
    hidInput(move(source.hidInput)),
    vkInput(move(source.vkInput)),
    swapChain(source.swapChain.release()), 
    renderTargetView(source.renderTargetView.release())
{
    source.hwnd = {};
    ASSUME(this != &source);
}

DX11Window &DX11Window::operator = (DX11Window &&source)
{
    ASSUME(this != &source);

    isChanged = source.isChanged;
    window = source.window;
    hwnd = move(source.hwnd);
    source.hwnd = {};
    controlsQueue = move(source.controlsQueue);
    hidInput = move(source.hidInput);
    vkInput = move(source.vkInput);
    swapChain = move(source.swapChain);
    renderTargetView = move(source.renderTargetView);

    return *this;
}

DX11Window::DX11Window(Window &window, ID3D11Device *device, LoggerWrapper &logger) : window(&window), device(device)
{
    string title = "RendererDX11: ";
    title += string_view(window.title.data(), window.title.size());

    RECT dim;
    dim.left = window.x;
    dim.right = window.x + window.width;
    dim.top = window.y;
    dim.bottom = window.y + window.height;

    hwnd = CreateSystemWindow(logger, title, window.isFullscreen, window.isNoBorders, window.isMaximized, dim, window.cursorType, this);

    if (hwnd)
    {
        hidInput = HIDInput();
        if (!hidInput->Register(*hwnd))
        {
            logger.Message(LogLevels::Error, "Failed to registed HID, using VK\n");
            hidInput = {};
            vkInput = VKInput();
        }
    }
}

bool DX11Window::MakeWindowAssociation(LoggerWrapper &logger)
{
    ASSUME(hwnd);

    if (!swapChain) // swap chain hasn't been created yet
    {
        ASSUME(currentWidth == 0 && currentHeight == 0 && renderTargetView == nullptr);

        DXGI_SWAP_CHAIN_DESC sd;
        sd.BufferDesc.Width = window->width;
        sd.BufferDesc.Height = window->height;
        sd.BufferDesc.RefreshRate.Numerator = 0;
        sd.BufferDesc.RefreshRate.Denominator = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 2;
        sd.OutputWindow = *hwnd;
        sd.Windowed = true;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        auto procError = [&]
        {
            SOFTBREAK;
        };

        COMUniquePtr<IDXGIDevice> dxgiDevice;
        if (HRESULT hresult = device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice) != S_OK)
        {
            logger.Message(LogLevels::Error, "CreateWindowAssociation: failed to get DXGI device with error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
            procError();
            return false;
        }

        COMUniquePtr<IDXGIAdapter> dxgiAdapter;
        if (HRESULT hresult = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter) != S_OK)
        {
            logger.Message(LogLevels::Error, "CreateWindowAssociation: failed to get DXGI adapter with error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
            procError();
            return false;
        }

        COMUniquePtr<IDXGIFactory> dxgiFactory;
        if (HRESULT hresult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory) != S_OK)
        {
            logger.Message(LogLevels::Error, "CreateWindowAssociation: failed to get DXGI factory with error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
            procError();
            return false;
        }

        if (HRESULT hresult = dxgiFactory->CreateSwapChain(device, &sd, (IDXGISwapChain **)&swapChain) != S_OK)
        {
            logger.Message(LogLevels::Error, "CreateWindowAssociation: create swap chain for the current window failed with error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
            procError();
            return false;
        }

        if (HRESULT hresult = dxgiFactory->MakeWindowAssociation(*hwnd, DXGI_MWA_NO_WINDOW_CHANGES) != S_OK)
        {
            logger.Message(LogLevels::Error, "CreateWindowAssociation: failed to make window association with error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
        }

        currentFullScreen = false;
        currentWidth = window->width;
        currentHeight = window->height;

        logger.Message(LogLevels::Info, "CreateWindowAssociation: created a new swap chain, size %u x %u\n", window->width, window->height);
    }

    return HandleSizeFullScreenChanges(logger);
}

bool DX11Window::CreateWindowRenderTargetView(LoggerWrapper &logger)
{
    if (swapChain == nullptr)
    {
        return false;
    }

    if (currentWidth != window->width || currentHeight != window->height)
    {
        renderTargetView = {};
    }

    if (renderTargetView != nullptr)
    {
        return false;
    }

    if (HRESULT hresult = swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0) != S_OK)
    {
        logger.Message(LogLevels::Error, "CreateWindowRenderTargetView: failed to resize swap chain's buffers with error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
        return false;
    }

    COMUniquePtr<ID3D11Texture2D> backBufferTexture;
    if (HRESULT hresult = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBufferTexture) != S_OK)
    {
        logger.Message(LogLevels::Error, "CreateWindowRenderTargetView: get back buffer for the current window's swap chain failed with error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
        return false;
    }

    if (HRESULT hresult = device->CreateRenderTargetView(backBufferTexture.get(), 0, (ID3D11RenderTargetView **)&renderTargetView) != S_OK)
    {
        logger.Message(LogLevels::Error, "CreateWindowRenderTargetView: create render target view for the current window failed with error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
        return false;
    }

    currentWidth = window->width;
    currentHeight = window->height;

    logger.Message(LogLevels::Info, "CreateWindowRenderTargetView: created swap chain's RTV, size is %ix%i\n", currentWidth, currentHeight);

    return true;
}

bool DX11Window::NotifyWindowResized(LoggerWrapper &logger)
{
    if (!_isWaitingForWindowResizeNotification)
    {
        return false;
    }

    return CreateWindowRenderTargetView(logger);
}

bool DX11Window::HandleSizeFullScreenChanges(LoggerWrapper &logger)
{
    HWND prevFocus = NULL;

    _isWaitingForWindowResizeNotification = true;

    DXGI_MODE_DESC mode;
    mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    mode.Width = window->width;
    mode.Height = window->height;
    mode.RefreshRate.Denominator = 0;
    mode.RefreshRate.Numerator = 0;
    mode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

    if (currentFullScreen != window->isFullscreen || currentWidth != window->width || currentHeight != window->height)
    {
        renderTargetView = {};

        if (HRESULT hresult = swapChain->ResizeTarget(&mode) != S_OK)
        {
            logger.Message(LogLevels::Error, "HandleWindowSizeFullScreenChanges: swap chain's ResizeTarget failed, error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
        }
        else
        {
            logger.Message(LogLevels::Info, "HandleWindowSizeFullScreenChanges: called ResizeTarget, size %u x %u\n", window->width, window->height);
            currentWidth = window->width;
            currentHeight = window->height;
        }
    }

    if (currentFullScreen != window->isFullscreen)
    {
        prevFocus = SetFocus(*hwnd);

        //  will trigger a WM_SIZE event that can change current window's size
        if (HRESULT hresult = swapChain->SetFullscreenState(window->isFullscreen ? TRUE : FALSE, 0) != S_OK)
        {
            logger.Message(LogLevels::Error, "HandleWindowSizeFullScreenChanges: failed to change fullscreen state of the swap chain, error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
        }
        else
        {
            currentFullScreen = window->isFullscreen;
            logger.Message(LogLevels::Info, "HandleWindowSizeFullScreenChanges: changed the swap chain's fullscreen state to %s\n", currentFullScreen ? "fullscreen" : "windowed");
        }

        mode.RefreshRate.Denominator = 0;
        mode.RefreshRate.Numerator = 0;
        if (HRESULT hresult = swapChain->ResizeTarget(&mode) != S_OK)
        {
            logger.Message(LogLevels::Error, "HandleWindowSizeFullScreenChanges: second swap chain's ResizeTarget failed, error 0x%h %s\n", hresult, ConvertDX11ErrToString(hresult));
        }
        else
        {
            logger.Message(LogLevels::Info, "HandleWindowSizeFullScreenChanges: called ResizeTarget, size %u x %u\n", window->width, window->height);
        }
    }

    _isWaitingForWindowResizeNotification = false;

    CreateWindowRenderTargetView(logger);

    if (prevFocus != NULL)
    {
        SetFocus(prevFocus);
    }

    return true;
}
