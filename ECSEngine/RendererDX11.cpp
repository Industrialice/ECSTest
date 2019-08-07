#include "PreHeader.hpp"
#include "RendererDX11.hpp"
#include "Components.hpp"
#include "CustomControlActions.hpp"
#include "WinHIDInput.hpp"
#include "WinVKInput.hpp"
#include "CameraTransform.hpp"

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

using namespace ECSEngine;

static optional<HWND> CreateSystemWindow(LoggerWrapper &logger, const string &title, bool isFullscreen, bool hideBorders, bool isMaximized, RECT &dimensions, Window::CursorTypet cursorType, void *userData);
static LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HCURSOR AcquireCursor(Window::CursorTypet type);
NOINLINE static const char *ConvertDirectXErrToString(HRESULT hresult);

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

    DX11Window(DX11Window &&source) noexcept;
    DX11Window &operator = (DX11Window &&source) noexcept;
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
	CameraTransform transform{};

    DX11Camera(EntityID id, const Camera &data, const CameraTransform &transform) : id(id), data(data), transform(transform) {}
    DX11Camera(DX11Camera &&) = default;
    DX11Camera &operator = (DX11Camera &&) = default;
};

class Cube
{
public:
	Cube() = default;

	Cube(LoggerWrapper &logger, ID3D11Device *device, ID3D11DeviceContext *context, ID3D11Buffer *uniformBuffer) : _device(device), _context(context), _uniformBuffer(uniformBuffer)
	{
		D3D11_RASTERIZER_DESC rsDesc{};
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.CullMode = D3D11_CULL_BACK;
		rsDesc.DepthBias = 0;
		rsDesc.DepthBiasClamp = 0.0f;
		rsDesc.DepthClipEnable = true;
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.FrontCounterClockwise = false;
		rsDesc.MultisampleEnable = false;
		rsDesc.ScissorEnable = false;
		rsDesc.SlopeScaledDepthBias = 0.0f;
		if (HRESULT result = _device->CreateRasterizerState(&rsDesc, AddressOfNaked(_rsState)); result != S_OK)
		{
			logger.Error("Failed to create rasterizer state, error %s\n", ConvertDirectXErrToString(result));
		}

		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = false;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (HRESULT result = _device->CreateBlendState(&blendDesc, AddressOfNaked(_blendState)); result != S_OK)
		{
			logger.Error("Failed to create blend state, error %s\n", ConvertDirectXErrToString(result));
		}

		const D3D11_DEPTH_STENCILOP_DESC stencilOpDesc = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};
		D3D11_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.BackFace = stencilOpDesc;
		dsDesc.DepthEnable = true;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.FrontFace = stencilOpDesc;
		dsDesc.StencilEnable = false;
		dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		if (HRESULT result = _device->CreateDepthStencilState(&dsDesc, AddressOfNaked(_dsState)); result != S_OK)
		{
			logger.Error("Failed to create depth stencil state, error %s\n", ConvertDirectXErrToString(result));
		}

		static constexpr f32 transparency = 0.5f;

		static constexpr Vector4 frontColor{1, 1, 1, transparency};
		static constexpr Vector4 upColor{0, 1, 1, transparency};
		static constexpr Vector4 backColor{0, 0, 1, transparency};
		static constexpr Vector4 downColor{1, 0, 0, transparency};
		static constexpr Vector4 leftColor{1, 1, 0, transparency};
		static constexpr Vector4 rightColor{1, 0, 1, transparency};

		struct Vertex
		{
			Vector3 position;
			Vector4 color;
		};

		static constexpr Vertex vertexArrayData[]
		{
			// front
			{{-0.5f, -0.5f, -0.5f}, frontColor},
			{{-0.5f, 0.5f, -0.5f}, frontColor},
			{{0.5f, -0.5f, -0.5f}, frontColor},
			{{0.5f, 0.5f, -0.5f}, frontColor},

			// up
			{{-0.5f, 0.5f, -0.5f}, upColor},
			{{-0.5f, 0.5f, 0.5f}, upColor},
			{{0.5f, 0.5f, -0.5f}, upColor},
			{{0.5f, 0.5f, 0.5f}, upColor},

			// back
			{{0.5f, -0.5f, 0.5f}, backColor},
			{{0.5f, 0.5f, 0.5f}, backColor},
			{{-0.5f, -0.5f, 0.5f}, backColor},
			{{-0.5f, 0.5f, 0.5f}, backColor},

			// down
			{{-0.5f, -0.5f, 0.5f}, downColor},
			{{-0.5f, -0.5f, -0.5f}, downColor},
			{{0.5f, -0.5f, 0.5f}, downColor},
			{{0.5f, -0.5f, -0.5f}, downColor},

			// left
			{{-0.5f, -0.5f, 0.5f}, leftColor},
			{{-0.5f, 0.5f, 0.5f}, leftColor},
			{{-0.5f, -0.5f, -0.5f}, leftColor},
			{{-0.5f, 0.5f, -0.5f}, leftColor},

			// right
			{{0.5f, -0.5f, -0.5f}, rightColor},
			{{0.5f, 0.5f, -0.5f}, rightColor},
			{{0.5f, -0.5f, 0.5f}, rightColor},
			{{0.5f, 0.5f, 0.5f}, rightColor},
		};

		D3D11_BUFFER_DESC bufDesc =
		{
			sizeof(vertexArrayData),
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_VERTEX_BUFFER,
			0,
			0,
			0
		};
		D3D11_SUBRESOURCE_DATA bufData;
		bufData.pSysMem = vertexArrayData;
		if (HRESULT result = _device->CreateBuffer(&bufDesc, &bufData, AddressOfNaked(_vertexBuffer)); result != S_OK)
		{
			logger.Error("Creating vertex buffer failed, error %s", ConvertDirectXErrToString(result));
		}

		ui16 indexes[36];
		for (ui32 index = 0; index < 6; ++index)
		{
			indexes[index * 6 + 0] = index * 4 + 0;
			indexes[index * 6 + 1] = index * 4 + 1;
			indexes[index * 6 + 2] = index * 4 + 3;

			indexes[index * 6 + 3] = index * 4 + 2;
			indexes[index * 6 + 4] = index * 4 + 0;
			indexes[index * 6 + 5] = index * 4 + 3;
		}

		bufDesc.ByteWidth = sizeof(indexes);
		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufData.pSysMem = indexes;
		if (HRESULT result = _device->CreateBuffer(&bufDesc, &bufData, AddressOfNaked(_indexBuffer)); result != S_OK)
		{
			logger.Error("Creating index buffer failed, error %s", ConvertDirectXErrToString(result));
		}

		const char *vsCode = TOSTR(
			cbuffer Uniforms : register(b0)
			{
				float4x4 ModelViewProjectionMatrix : packoffset(c0);
			}

			void VSMain(float4 pos : POSITION, float4 color : COLOR, out float4 outPos : SV_POSITION, out float4 outColor : COLOR)
			{
				outPos = mul(pos, ModelViewProjectionMatrix);
				outColor = color;
			}
		);

		const char *psCode = TOSTR(
			float4 PSMain(float4 pos : SV_POSITION, float4 color : COLOR) : SV_TARGET
			{
				return color;
			}
		);

		#if defined(DEBUG)
			const UINT compileFlags = D3DCOMPILE_DEBUG;
		#else
			const UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_SKIP_VALIDATION;
		#endif

		D3D_FEATURE_LEVEL featureLevel = _device->GetFeatureLevel();
		auto featureLevelToTarget = [featureLevel]
		{
			switch (featureLevel)
			{
			case D3D_FEATURE_LEVEL_9_1:
			case D3D_FEATURE_LEVEL_9_2:
				return "xs_4_0_level_9_1";
			case D3D_FEATURE_LEVEL_9_3:
				return "xs_4_0_level_9_3";
			case D3D_FEATURE_LEVEL_10_0:
				return "xs_4_0";
			case D3D_FEATURE_LEVEL_10_1:
				return "xs_4_1";
			case D3D_FEATURE_LEVEL_11_0:
			case D3D_FEATURE_LEVEL_11_1:
			case D3D_FEATURE_LEVEL_12_0:
			case D3D_FEATURE_LEVEL_12_1:
			default:
				return "xs_5_0";
			}
		};

		const char *featureLevelShaderStr = featureLevelToTarget();

		COMUniquePtr<ID3DBlob> compiledShader, errors;
		char shaderTarget[32];
		strcpy_s(shaderTarget, featureLevelShaderStr);
		shaderTarget[0] = 'v';
		if (HRESULT result = D3DCompile(vsCode, strlen(vsCode), 0, nullptr, 0, "VSMain", shaderTarget, compileFlags, 0, AddressOfNaked(compiledShader), AddressOfNaked(errors)); result != S_OK)
		{
			if (errors)
			{
				logger.Error("Vertex shader compilation failed, error %s, compile errors %*s\n", ConvertDirectXErrToString(result), static_cast<i32>(errors->GetBufferSize()), static_cast<char *>(errors->GetBufferPointer()));
			}
			else
			{
				logger.Error("Vertex shader compilation failed, error %s\n", ConvertDirectXErrToString(result));
			}
		}

		if (HRESULT result = _device->CreateVertexShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, AddressOfNaked(_vertexShader)); result != S_OK)
		{
			logger.Error("Vertex shader creation failed, error %s\n", ConvertDirectXErrToString(result));
		}

		static constexpr D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		if (HRESULT result = _device->CreateInputLayout(inputLayoutDesc, CountOf(inputLayoutDesc), compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), AddressOfNaked(_inputLayout)); result != S_OK)
		{
			logger.Error("Input layout creation failed, error %s\n", ConvertDirectXErrToString(result));
		}

		compiledShader = {};
		errors = {};

		shaderTarget[0] = 'p';
		if (HRESULT result = D3DCompile(psCode, strlen(psCode), 0, nullptr, 0, "PSMain", shaderTarget, compileFlags, 0, AddressOfNaked(compiledShader), AddressOfNaked(errors)); result != S_OK)
		{
			if (errors)
			{
				logger.Error("Pixel shader compilation failed, error %s, compile errors %*s\n", ConvertDirectXErrToString(result), static_cast<i32>(errors->GetBufferSize()), static_cast<char *>(errors->GetBufferPointer()));
			}
			else
			{
				logger.Error("Pixel shader compilation failed, error %s\n", ConvertDirectXErrToString(result));
			}
		}

		if (HRESULT result = _device->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, AddressOfNaked(_pixelShader)); result != S_OK)
		{
			logger.Error("Pixel shader creation failed, error %s\n", ConvertDirectXErrToString(result));
		}

		logger.Info("Finished creating cube\n");
	}

	void Draw(System::Environment &env, const Matrix4x4 &viewProjectionMatrix)
	{
		static constexpr UINT strides[] = {sizeof(Vector3) + sizeof(Vector4)};
		static constexpr UINT offsets[] = {0};

		D3D11_MAPPED_SUBRESOURCE mapped;
		if (HRESULT result = _context->Map(_uniformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped); result != S_OK)
		{
			env.logger.Error("Uniform buffer mapping failed, error %s\n", ConvertDirectXErrToString(result));
		}

		Matrix4x3 modelMatrix = Matrix4x3::CreateRTS(Vector3(0, env.timeSinceStarted.ToSec(), 0), Vector3(0, 0, 1));

		Matrix4x4 mvp = modelMatrix * viewProjectionMatrix;
		mvp.Transpose();
		MemOps::Copy(static_cast<Matrix4x4 *>(mapped.pData), &mvp, 1);

		_context->Unmap(_uniformBuffer, 0);

		_context->RSSetState(_rsState.get());
		_context->OMSetBlendState(_blendState.get(), nullptr, ui32_max);
		_context->OMSetDepthStencilState(_dsState.get(), ui32_max);
		_context->IASetVertexBuffers(0, 1, AddressOfNaked(_vertexBuffer), strides, offsets);
		_context->IASetIndexBuffer(_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
		_context->PSSetShader(_pixelShader.get(), nullptr, 0);
		_context->VSSetShader(_vertexShader.get(), nullptr, 0);
		_context->IASetInputLayout(_inputLayout.get());
		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_context->DrawIndexed(36, 0, 0);
	}

private:
	COMUniquePtr<ID3D11InputLayout> _inputLayout{};
	COMUniquePtr<ID3D11VertexShader> _vertexShader{};
	COMUniquePtr<ID3D11PixelShader> _pixelShader{};
	COMUniquePtr<ID3D11Buffer> _vertexBuffer{};
	COMUniquePtr<ID3D11Buffer> _indexBuffer{};
	COMUniquePtr<ID3D11RasterizerState> _rsState{};
	COMUniquePtr<ID3D11BlendState> _blendState{};
	COMUniquePtr<ID3D11DepthStencilState> _dsState{};
	ID3D11Device *_device{};
	ID3D11DeviceContext *_context{};
	ID3D11Buffer *_uniformBuffer{};
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
				AddCamera(entry.entityID, *camera, {entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation});
            }
        }
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
    {
        if (stream.Type() == Camera::GetTypeId())
        {
            for (auto &entry : stream)
            {
                AddCamera(entry.entityID, entry.added.Cast<Camera>(), {entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation});
            }
        }
        else if (stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId())
        {
			for (auto &entry : stream)
			{
				auto *camera = entry.FindComponent<Camera>();
				if (camera)
				{
					AddCamera(entry.entityID, *camera, {entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation});
				}
			}
        }
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
    {
        for (const auto &entry : stream.Enumerate<Camera>())
        {
			CameraChanged(entry.entityID, entry.component);
        }
		for (const auto &entry : stream.Enumerate<Position>())
		{
			CameraPositionChanged(entry.entityID, entry.component);
		}
		for (const auto &entry : stream.Enumerate<Rotation>())
		{
			CameraRotationChanged(entry.entityID, entry.component);
		}

		ASSUME(stream.Type() == Camera::GetTypeId() || stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId());
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
    {
		for (const auto &entry : stream.Enumerate<Camera>())
		{
			RemoveCamera(entry.entityID);
		}
		for (const auto &entry : stream.Enumerate<Position>())
		{
			RemoveCamera(entry.entityID);
		}
		for (const auto &entry : stream.Enumerate<Rotation>())
		{
			RemoveCamera(entry.entityID);
		}

		ASSUME(stream.Type() == Camera::GetTypeId() || stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId());
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
		ASSUME(env.targetSystem == GetTypeId());

        for (auto &camera : _cameras)
        {
            for (uiw windowIndex = 0; windowIndex < camera.data.rt.size(); ++windowIndex)
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

						_context->OMSetRenderTargets(1, AddressOfNaked(dxWindow.renderTargetView), nullptr);

						D3D11_VIEWPORT viewport{};
						viewport.Width = static_cast<f32>(window->width);
						viewport.Height = static_cast<f32>(window->height);
						viewport.MaxDepth = 1.0f;
						viewport.MinDepth = 0.0f;
						viewport.TopLeftX = 0.0f;
						viewport.TopLeftY = 0.0;
						_context->RSSetViewports(1, &viewport);

						auto projectionMatrix = Matrix4x4::CreatePerspectiveProjection(DegToRad(75.0f), static_cast<f32>(window->width) / static_cast<f32>(window->height), 0.1f, 1000.0f, ProjectionTarget::D3DAndMetal);
						Matrix4x4 viewProjectionMatrix = camera.transform.ViewMatrix() * projectionMatrix;

						_cube.Draw(env, viewProjectionMatrix);

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

            for (uiw windowIndex = 0; windowIndex < camera.data.rt.size(); ++windowIndex)
            {
                isChanged |= camera.windows[windowIndex].isChanged;
                camera.windows[windowIndex].isChanged = false;
                env.keyController->Dispatch(camera.windows[windowIndex].controlsQueue);
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
        env.logger.Info("Initializing\n");

        constexpr ui32 adaptersMask = ui32_max;
        constexpr D3D_FEATURE_LEVEL maxFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        COMUniquePtr<IDXGIFactory1> dxgiFactory;
        HRESULT dxgiFactoryResult = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(AddressOfNaked(dxgiFactory)));
        if (FAILED(dxgiFactoryResult))
        {
            env.logger.Error("Failed to create DXGI factory, error %s\n", ConvertDirectXErrToString(dxgiFactoryResult));
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
            env.logger.Error("Adapters list is empty, try another adaptersMask\n");
            return;
        }

        UINT createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

    #if defined(DEBUG)
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

        vector<D3D_FEATURE_LEVEL> featureLevels{D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1};
		featureLevels.erase(std::remove_if(featureLevels.begin(), featureLevels.end(), [maxFeatureLevel](D3D_FEATURE_LEVEL level) { return level > maxFeatureLevel; }), featureLevels.end());

		auto createDevice = [&]
		{
			return D3D11CreateDevice(
				adapters[0].get(),
				D3D_DRIVER_TYPE_UNKNOWN,
				NULL,
				createDeviceFlags,
				featureLevels.data(),
				(UINT)featureLevels.size(),
				D3D11_SDK_VERSION,
				AddressOfNaked(_device),
				&_featureLevel,
				AddressOfNaked(_context));
		};

		HRESULT result = createDevice();
		if (result == DXGI_ERROR_SDK_COMPONENT_MISSING)
		{
			env.logger.Warning("Failed to create device, error DXGI_ERROR_SDK_COMPONENT_MISSING, trying to create without debug layers\n");
			createDeviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
			result = createDevice();
		}

        if (FAILED(result))
        {
            env.logger.Error("Failed to create device, error %s\n", ConvertDirectXErrToString(result));
            return;
        }

        _maxSupportedFeatureLevel = _device->GetFeatureLevel(); // TODO: this can be off

        ASSUME(_device != nullptr && _context != nullptr);

		D3D11_BUFFER_DESC bd;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.ByteWidth = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		result = _device->CreateBuffer(&bd, nullptr, AddressOfNaked(_uniformBuffer));
		if (FAILED(result))
		{
			env.logger.Error("Failed to create uniform buffer, error %s\n", ConvertDirectXErrToString(result));
			return;
		}

		_context->VSSetConstantBuffers(0, 1, AddressOfNaked(_uniformBuffer));
		_context->PSSetConstantBuffers(0, 1, AddressOfNaked(_uniformBuffer));
		if (_featureLevel >= D3D_FEATURE_LEVEL_10_0)
		{
			_context->GSSetConstantBuffers(0, 1, AddressOfNaked(_uniformBuffer));
			_context->CSSetConstantBuffers(0, 1, AddressOfNaked(_uniformBuffer));

			if (_featureLevel >= D3D_FEATURE_LEVEL_11_0)
			{
				_context->DSSetConstantBuffers(0, 1, AddressOfNaked(_uniformBuffer));
				_context->HSSetConstantBuffers(0, 1, AddressOfNaked(_uniformBuffer));
			}
		}

		_cube = Cube(env.logger, _device.get(), _context.get(), _uniformBuffer.get());
        
        env.logger.Info("Finished initialization\n");
    }

    virtual void OnDestroy(Environment &env) override
    {
        env.logger.Info("Deinitializing\n");

        _context = {};
        _device = {};

        env.logger.Info("Finished deinitialization\n");
    }

private:
    void AddCamera(EntityID id, const Camera &data, const CameraTransform &transform)
    {
        _cameras.emplace_front(id, data, transform);
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
                for (uiw windowIndex = 0; windowIndex < camera.data.rt.size(); ++windowIndex)
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

	void CameraPositionChanged(EntityID id, const Position &newPosition)
	{
		for (auto &camera : _cameras)
		{
			if (camera.id == id)
			{
				camera.transform.Position(newPosition.position);
				return;
			}
		}

		UNREACHABLE;
	}

	void CameraRotationChanged(EntityID id, const Rotation &newRotation)
	{
		for (auto &camera : _cameras)
		{
			if (camera.id == id)
			{
				camera.transform.Rotation(newRotation.rotation.ToEuler());
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
	COMUniquePtr<ID3D11Buffer> _uniformBuffer{};
	Cube _cube{};
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
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    wc.lpszMenuName = 0;
    wc.lpszClassName = className.c_str();

    if (!RegisterClassA(&wc))
    {
        logger.Critical("Failed to register class\n");
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
        logger.Critical("Failed to create requested window\n");
        return nullopt;
    }

    SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userData));

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto data = reinterpret_cast<DX11Window *>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
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

NOINLINE const char *ConvertDirectXErrToString(HRESULT hresult)
{
    switch (hresult)
    {
	#define CASE(Val) case Val: return TOSTR(#Val);
	CASE(D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS)
	CASE(D3D10_ERROR_FILE_NOT_FOUND)
	CASE(D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS)
	CASE(D3D11_ERROR_FILE_NOT_FOUND)
	CASE(D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS)
	CASE(D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD)
	CASE(D3D12_ERROR_ADAPTER_NOT_FOUND)
	CASE(D3D12_ERROR_DRIVER_VERSION_MISMATCH)
    CASE(DXGI_STATUS_OCCLUDED)
    CASE(DXGI_STATUS_CLIPPED)
    CASE(DXGI_STATUS_NO_REDIRECTION)
    CASE(DXGI_STATUS_NO_DESKTOP_ACCESS)
    CASE(DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE)
    CASE(DXGI_STATUS_MODE_CHANGED)
    CASE(DXGI_STATUS_MODE_CHANGE_IN_PROGRESS)
    CASE(DXGI_ERROR_INVALID_CALL)
    CASE(DXGI_ERROR_NOT_FOUND)
    CASE(DXGI_ERROR_MORE_DATA)
    CASE(DXGI_ERROR_UNSUPPORTED)
    CASE(DXGI_ERROR_DEVICE_REMOVED)
    CASE(DXGI_ERROR_DEVICE_HUNG)
    CASE(DXGI_ERROR_DEVICE_RESET)
    CASE(DXGI_ERROR_WAS_STILL_DRAWING)
	CASE(DXGI_ERROR_FRAME_STATISTICS_DISJOINT)
	CASE(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE)
	CASE(DXGI_ERROR_DRIVER_INTERNAL_ERROR)
	CASE(DXGI_ERROR_NONEXCLUSIVE)
	CASE(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
	CASE(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED)
	CASE(DXGI_ERROR_REMOTE_OUTOFMEMORY)
	CASE(DXGI_ERROR_ACCESS_LOST)
	CASE(DXGI_ERROR_WAIT_TIMEOUT)
	CASE(DXGI_ERROR_SESSION_DISCONNECTED)
	CASE(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE)
	CASE(DXGI_ERROR_CANNOT_PROTECT_CONTENT)
	CASE(DXGI_ERROR_ACCESS_DENIED)
	CASE(DXGI_ERROR_NAME_ALREADY_EXISTS)
	CASE(DXGI_ERROR_SDK_COMPONENT_MISSING)
	CASE(DXGI_ERROR_NOT_CURRENT)
	CASE(DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY)
	CASE(DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION)
	CASE(DXGI_ERROR_NON_COMPOSITED_UI)
	CASE(DXGI_STATUS_UNOCCLUDED)
	CASE(DXGI_STATUS_DDA_WAS_STILL_DRAWING)
	CASE(DXGI_ERROR_MODE_CHANGE_IN_PROGRESS)
	CASE(DXGI_STATUS_PRESENT_REQUIRED)
	CASE(DXGI_ERROR_CACHE_CORRUPT)
	CASE(DXGI_ERROR_CACHE_FULL)
	CASE(DXGI_ERROR_CACHE_HASH_COLLISION)
	CASE(DXGI_ERROR_ALREADY_EXISTS)
	CASE(DXGI_DDI_ERR_WASSTILLDRAWING)
	CASE(DXGI_DDI_ERR_UNSUPPORTED)
	CASE(DXGI_DDI_ERR_NONEXCLUSIVE)
    CASE(E_FAIL)
    CASE(E_INVALIDARG)
    CASE(E_OUTOFMEMORY)
    CASE(S_FALSE)
    default:
        return "unidentified error";
	#undef CASE
    }
}

DX11Window::DX11Window(DX11Window &&source) noexcept : 
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

DX11Window &DX11Window::operator = (DX11Window &&source) noexcept
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
            logger.Error("Failed to registed HID, using VK\n");
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
        if (HRESULT hresult = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(AddressOfNaked(dxgiDevice))); hresult != S_OK)
        {
            logger.Error("CreateWindowAssociation: failed to get DXGI device with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
            procError();
            return false;
        }

        COMUniquePtr<IDXGIAdapter> dxgiAdapter;
        if (HRESULT hresult = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void **>(AddressOfNaked(dxgiAdapter))); hresult != S_OK)
        {
            logger.Error("CreateWindowAssociation: failed to get DXGI adapter with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
            procError();
            return false;
        }

        COMUniquePtr<IDXGIFactory> dxgiFactory;
        if (HRESULT hresult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void **>(AddressOfNaked(dxgiFactory))); hresult != S_OK)
        {
            logger.Error("CreateWindowAssociation: failed to get DXGI factory with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
            procError();
            return false;
        }

        if (HRESULT hresult = dxgiFactory->CreateSwapChain(device, &sd, reinterpret_cast<IDXGISwapChain **>(AddressOfNaked(swapChain))); hresult != S_OK)
        {
            logger.Error("CreateWindowAssociation: create swap chain for the current window failed with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
            procError();
            return false;
        }

        if (HRESULT hresult = dxgiFactory->MakeWindowAssociation(*hwnd, DXGI_MWA_NO_WINDOW_CHANGES); hresult != S_OK)
        {
            logger.Error("CreateWindowAssociation: failed to make window association with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
        }

        currentFullScreen = false;
        currentWidth = window->width;
        currentHeight = window->height;

        logger.Info("CreateWindowAssociation: created a new swap chain, size %u x %u\n", window->width, window->height);
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

    if (HRESULT hresult = swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0); hresult != S_OK)
    {
        logger.Error("CreateWindowRenderTargetView: failed to resize swap chain's buffers with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
        return false;
    }

    COMUniquePtr<ID3D11Texture2D> backBufferTexture;
    if (HRESULT hresult = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(AddressOfNaked(backBufferTexture))); hresult != S_OK)
    {
        logger.Error("CreateWindowRenderTargetView: get back buffer for the current window's swap chain failed with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
        return false;
    }

    if (HRESULT hresult = device->CreateRenderTargetView(backBufferTexture.get(), 0, AddressOfNaked(renderTargetView)); hresult != S_OK)
    {
        logger.Error("CreateWindowRenderTargetView: create render target view for the current window failed with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
        return false;
    }

    currentWidth = window->width;
    currentHeight = window->height;

    logger.Info("CreateWindowRenderTargetView: created swap chain's RTV, size is %ix%i\n", currentWidth, currentHeight);

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

        if (HRESULT hresult = swapChain->ResizeTarget(&mode); hresult != S_OK)
        {
            logger.Error("HandleWindowSizeFullScreenChanges: swap chain's ResizeTarget failed, error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
        }
        else
        {
            logger.Info("HandleWindowSizeFullScreenChanges: called ResizeTarget, size %u x %u\n", window->width, window->height);
            currentWidth = window->width;
            currentHeight = window->height;
        }
    }

    if (currentFullScreen != window->isFullscreen)
    {
        prevFocus = SetFocus(*hwnd);

        //  will trigger a WM_SIZE event that can change current window's size
        if (HRESULT hresult = swapChain->SetFullscreenState(window->isFullscreen ? TRUE : FALSE, 0); hresult != S_OK)
        {
            logger.Error("HandleWindowSizeFullScreenChanges: failed to change fullscreen state of the swap chain, error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
        }
        else
        {
            currentFullScreen = window->isFullscreen;
            logger.Info("HandleWindowSizeFullScreenChanges: changed the swap chain's fullscreen state to %s\n", currentFullScreen ? "fullscreen" : "windowed");
        }

        mode.RefreshRate.Denominator = 0;
        mode.RefreshRate.Numerator = 0;
        if (HRESULT hresult = swapChain->ResizeTarget(&mode); hresult != S_OK)
        {
            logger.Error("HandleWindowSizeFullScreenChanges: second swap chain's ResizeTarget failed, error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
        }
        else
        {
            logger.Info("HandleWindowSizeFullScreenChanges: called ResizeTarget, size %u x %u\n", window->width, window->height);
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
