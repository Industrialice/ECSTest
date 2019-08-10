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
static DXGI_FORMAT ColorFormatToDirectX(ColorFormatt format);
static uiw ColorFormatSizeOf(ColorFormatt format);

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

struct DX11DepthStencilBuffer
{
	ID3D11Device *device{};
	COMUniquePtr<ID3D11DepthStencilView> depthStencilView{};
	Camera::DepthBufferFormat depthBufferFormat{};
	ui32 width{}, height{};

public:
	DX11DepthStencilBuffer() = default;
	DX11DepthStencilBuffer(ID3D11Device *device, Camera::DepthBufferFormat depthBufferFormat);
	DX11DepthStencilBuffer(DX11DepthStencilBuffer &&) = default;
	DX11DepthStencilBuffer &operator = (DX11DepthStencilBuffer &&) = default;
	bool Resize(LoggerWrapper &logger, ui32 width, ui32 height);
};

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
	shared_ptr<DX11DepthStencilBuffer> depthStencilBuffer{};

private:
    bool _isWaitingForWindowResizeNotification = false;

public:
    ~DX11Window()
    {
        renderTargetView = {};
		depthStencilBuffer = {};

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

	DX11Window(Window &window, const shared_ptr<DX11DepthStencilBuffer> &depthStencilBuffer, ID3D11Device *device, LoggerWrapper &logger);
    DX11Window(DX11Window &&source) noexcept;
	DX11Window &operator = (DX11Window &&source) noexcept;
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
	shared_ptr<DX11DepthStencilBuffer> depthStencilBuffer{};

	DX11Camera(EntityID id, ID3D11Device *device, const Camera &data, const CameraTransform &transform);
    DX11Camera(DX11Camera &&) = default;
    DX11Camera &operator = (DX11Camera &&) = default;
};

class LayoutManager
{
	static auto DescToTie(const D3D11_INPUT_ELEMENT_DESC &desc)
	{
		return std::tie(desc.Format, desc.AlignedByteOffset, desc.InputSlot, desc.InputSlotClass, desc.InstanceDataStepRate, desc.SemanticIndex, desc.SemanticName);
	}

	struct StoredElement
	{
		ui32 hash{};
		vector<D3D11_INPUT_ELEMENT_DESC> descs{};
		COMUniquePtr<ID3D11InputLayout> inputLayout{};
	};

public:
	ID3D11InputLayout *Create(LoggerWrapper &logger, ID3D11Device *device, Array<D3D11_INPUT_ELEMENT_DESC> descs, Array<byte> shaderCode)
	{
		ui32 hash = descs.size();
		for (uiw index = 0; index < descs.size(); ++index)
		{
			hash += Hash::FNVHash<Hash::Precision::P32>(descs[index].SemanticName) * (index + 1);
		}

		auto isEqual = [hash, &descs](const StoredElement &stored)
		{
			if (hash != stored.hash)
			{
				return false;
			}

			if (descs.size() != stored.descs.size())
			{
				return false;
			}

			for (uiw index = 0; index < descs.size(); ++index)
			{
				if (DescToTie(descs[index]) != DescToTie(stored.descs[index]))
				{
					return false;
				}
			}

			return true;
		};

		for (const auto &stored : _storage)
		{
			if (isEqual(stored))
			{
				return stored.inputLayout.get();
			}
		}

		StoredElement element;

		if (HRESULT result = device->CreateInputLayout(descs.data(), descs.size(), shaderCode.data(), shaderCode.size(), AddressOfNaked(element.inputLayout)); result != S_OK)
		{
			logger.Error("LayoutManager.Create -> creating input layout failed, error %s\n", ConvertDirectXErrToString(result));
			return nullptr;
		}

		element.descs.assign(descs.begin(), descs.end());
		element.hash = hash;

		_storage.emplace_back(move(element));
		logger.Info("LayoutManager.Create -> created new input layout\n");

		return _storage.back().inputLayout.get();
	}

private:
	vector<StoredElement> _storage{};
};

class MeshResource
{
public:
	struct SubmeshInfo
	{
		ui32 indexCount{};
		ui32 startIndex{};
	};

	MeshResource() = default;
	MeshResource(MeshResource &&) = default;
	MeshResource &operator = (MeshResource &&) = default;

	MeshResource(LoggerWrapper &logger, ID3D11Device *device, const MeshAsset &mesh, LayoutManager &layoutManager, Array<byte> shaderCodeForLayout, bool &isMeshCreated)
	{
		isMeshCreated = false;

		D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		if (mesh.desc.vertexAttributes.size() > D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)
		{
			SOFTBREAK;
			return;
		}

		_stride = 0;
		for (uiw attrIndex = 0; attrIndex < mesh.desc.vertexAttributes.size(); ++attrIndex)
		{
			ColorFormatt format = mesh.desc.vertexAttributes[attrIndex].type;

			_stride += ColorFormatSizeOf(format);

			inputLayoutDesc[attrIndex].Format = ColorFormatToDirectX(format);
			inputLayoutDesc[attrIndex].InputSlot = 0;
			inputLayoutDesc[attrIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			inputLayoutDesc[attrIndex].InstanceDataStepRate = 0;
			inputLayoutDesc[attrIndex].SemanticIndex = 0;
			inputLayoutDesc[attrIndex].SemanticName = mesh.desc.vertexAttributes[attrIndex].name.c_str();
			inputLayoutDesc[attrIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		}

		_inputLayout = layoutManager.Create(logger, device, Array(inputLayoutDesc, mesh.desc.vertexAttributes.size()), shaderCodeForLayout);
		if (!_inputLayout)
		{
			return;
		}

		ui16 vertexCount = 0;
		ui32 indexCount = 0;
		for (const auto &subMesh : mesh.desc.subMeshInfos)
		{
			_submeshInfos.push_back({.indexCount = subMesh.indexCount, .startIndex = indexCount});

			ASSUME(subMesh.vertexCount && subMesh.indexCount);
			ASSUME(subMesh.vertexCount <= ui16_max);
			ASSUME(vertexCount < vertexCount + subMesh.vertexCount); // make sure there're no overflows

			vertexCount += subMesh.vertexCount;
			indexCount += subMesh.indexCount;
		}

		D3D11_BUFFER_DESC bufDesc =
		{
			vertexCount * _stride,
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_VERTEX_BUFFER,
			0,
			0,
			0
		};
		D3D11_SUBRESOURCE_DATA bufData;
		bufData.pSysMem = mesh.data.get();
		if (HRESULT result = device->CreateBuffer(&bufDesc, &bufData, AddressOfNaked(_vertexBuffer)); result != S_OK)
		{
			logger.Error("MeshResource -> creating vertex buffer failed, error %s\n", ConvertDirectXErrToString(result));
			return;
		}

		bufDesc.ByteWidth = indexCount * sizeof(ui16);
		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufData.pSysMem = mesh.data.get() + vertexCount * _stride;
		if (HRESULT result = device->CreateBuffer(&bufDesc, &bufData, AddressOfNaked(_indexBuffer)); result != S_OK)
		{
			logger.Error("MeshResource -> creating index buffer failed, error %s\n", ConvertDirectXErrToString(result));
			return;
		}

		isMeshCreated = true;
	}

	SubmeshInfo GetSubmeshInfo(uiw submeshIndex) const
	{
		return _submeshInfos[submeshIndex];
	}

	uiw GetSubmeshCount() const
	{
		return _submeshInfos.size();
	}

	void SetPipeline(ID3D11DeviceContext *context)
	{
		ASSUME(_inputLayout && _vertexBuffer && _indexBuffer);

		UINT strides[] = {_stride};
		UINT offsets[] = {0};

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(_inputLayout);
		context->IASetIndexBuffer(_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
		context->IASetVertexBuffers(0, 1, AddressOfNaked(_vertexBuffer), strides, offsets);
	}

private:
	ui32 _stride{};
	ID3D11InputLayout *_inputLayout{};
	COMUniquePtr<ID3D11Buffer> _vertexBuffer{};
	COMUniquePtr<ID3D11Buffer> _indexBuffer{};
	vector<SubmeshInfo> _submeshInfos{};
};

class RenderingMaterial
{
public:
	RenderingMaterial() = default;
	RenderingMaterial(RenderingMaterial &&) = default;
	RenderingMaterial &operator = (RenderingMaterial &&) = default;

	RenderingMaterial(LoggerWrapper &logger, ID3D11Device *device, bool &isMaterialCreated)
	{
		isMaterialCreated = false;

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
		if (HRESULT result = device->CreateRasterizerState(&rsDesc, AddressOfNaked(_rsState)); result != S_OK)
		{
			logger.Error("Failed to create rasterizer state, error %s\n", ConvertDirectXErrToString(result));
			return;
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
		if (HRESULT result = device->CreateBlendState(&blendDesc, AddressOfNaked(_blendState)); result != S_OK)
		{
			logger.Error("Failed to create blend state, error %s\n", ConvertDirectXErrToString(result));
			return;
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
		if (HRESULT result = device->CreateDepthStencilState(&dsDesc, AddressOfNaked(_dsState)); result != S_OK)
		{
			logger.Error("Failed to create depth stencil state, error %s\n", ConvertDirectXErrToString(result));
			return;
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

		D3D_FEATURE_LEVEL featureLevel = device->GetFeatureLevel();
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

		COMUniquePtr<ID3DBlob> errors;
		char shaderTarget[32];
		strcpy_s(shaderTarget, featureLevelShaderStr);
		shaderTarget[0] = 'v';
		if (HRESULT result = D3DCompile(vsCode, strlen(vsCode), 0, nullptr, 0, "VSMain", shaderTarget, compileFlags, 0, AddressOfNaked(_vertexShaderCode), AddressOfNaked(errors)); result != S_OK)
		{
			if (errors)
			{
				logger.Error("Vertex shader compilation failed, error %s, compile errors %*s\n", ConvertDirectXErrToString(result), static_cast<i32>(errors->GetBufferSize()), static_cast<char *>(errors->GetBufferPointer()));
			}
			else
			{
				logger.Error("Vertex shader compilation failed, error %s\n", ConvertDirectXErrToString(result));
			}
			return;
		}

		if (HRESULT result = device->CreateVertexShader(_vertexShaderCode->GetBufferPointer(), _vertexShaderCode->GetBufferSize(), nullptr, AddressOfNaked(_vertexShader)); result != S_OK)
		{
			logger.Error("Vertex shader creation failed, error %s\n", ConvertDirectXErrToString(result));
			return;
		}

		COMUniquePtr<ID3DBlob> pixelShaderCode;
		errors = {};
		shaderTarget[0] = 'p';
		if (HRESULT result = D3DCompile(psCode, strlen(psCode), 0, nullptr, 0, "PSMain", shaderTarget, compileFlags, 0, AddressOfNaked(pixelShaderCode), AddressOfNaked(errors)); result != S_OK)
		{
			if (errors)
			{
				logger.Error("Pixel shader compilation failed, error %s, compile errors %*s\n", ConvertDirectXErrToString(result), static_cast<i32>(errors->GetBufferSize()), static_cast<char *>(errors->GetBufferPointer()));
			}
			else
			{
				logger.Error("Pixel shader compilation failed, error %s\n", ConvertDirectXErrToString(result));
			}
			return;
		}

		if (HRESULT result = device->CreatePixelShader(pixelShaderCode->GetBufferPointer(), pixelShaderCode->GetBufferSize(), nullptr, AddressOfNaked(_pixelShader)); result != S_OK)
		{
			logger.Error("Pixel shader creation failed, error %s\n", ConvertDirectXErrToString(result));
			return;
		}

		isMaterialCreated = true;
		logger.Info("Finished creating material\n");
	}

	void Draw(ID3D11DeviceContext *context, ID3D11Buffer *uniformBuffer, System::Environment &env, const Matrix4x3 &modelMatrix, const Matrix4x4 &viewProjectionMatrix, ui32 indexCount, ui32 indexBufferOffset)
	{
		static constexpr UINT strides[] = {sizeof(Vector3) + sizeof(Vector4)};
		static constexpr UINT offsets[] = {0};

		D3D11_MAPPED_SUBRESOURCE mapped;
		if (HRESULT result = context->Map(uniformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped); result != S_OK)
		{
			env.logger.Error("Uniform buffer mapping failed, error %s\n", ConvertDirectXErrToString(result));
		}

		Matrix4x4 mvp = modelMatrix * viewProjectionMatrix;
		mvp.Transpose();
		MemOps::Copy(static_cast<Matrix4x4 *>(mapped.pData), &mvp, 1);

		context->Unmap(uniformBuffer, 0);

		context->RSSetState(_rsState.get());
		context->OMSetBlendState(_blendState.get(), nullptr, ui32_max);
		context->OMSetDepthStencilState(_dsState.get(), ui32_max);
		context->PSSetShader(_pixelShader.get(), nullptr, 0);
		context->VSSetShader(_vertexShader.get(), nullptr, 0);

		context->DrawIndexed(indexCount, 0, indexBufferOffset);
	}

	ID3DBlob *VertexShaderCode()
	{
		return _vertexShaderCode.get();
	}

private:
	COMUniquePtr<ID3D11VertexShader> _vertexShader{};
	COMUniquePtr<ID3D11PixelShader> _pixelShader{};
	COMUniquePtr<ID3D11RasterizerState> _rsState{};
	COMUniquePtr<ID3D11BlendState> _blendState{};
	COMUniquePtr<ID3D11DepthStencilState> _dsState{};
	COMUniquePtr<ID3DBlob> _vertexShaderCode{};
};

class MeshRendererObject
{
public:
	MeshRendererObject() = default;

	MeshRendererObject(System::Environment &env, ID3D11Device *device, LayoutManager &layoutManager, const MeshRenderer &meshRenderer)
	{
		bool isMaterialCreated = false;
		_material = RenderingMaterial(env.logger, device, isMaterialCreated);
		if (!isMaterialCreated)
		{
			return;
		}

		const MeshAsset *meshAsset = env.assetsManager.Load<MeshAsset>(meshRenderer.mesh);
		if (!meshAsset)
		{
			env.logger.Error("MeshRendererObject failed to load mesh asset\n");
			return;
		}

		bool isMeshCreated = false;
		_mesh = MeshResource(env.logger, device, *meshAsset, layoutManager, Array<byte>(static_cast<byte *>(_material.VertexShaderCode()->GetBufferPointer()), _material.VertexShaderCode()->GetBufferSize()), isMeshCreated);
		if (!isMaterialCreated)
		{
			return;
		}

		env.logger.Info("Finished creating cube\n");
	}

	void Draw(ID3D11DeviceContext *context, ID3D11Buffer *uniformBuffer, System::Environment &env, const Matrix4x4 &viewProjectionMatrix)
	{
		if (!_modelMatrix)
		{
			_modelMatrix = Matrix4x3::CreateRTS(_rotation, _position);
		}

		_mesh.SetPipeline(context);

		for (uiw meshIndex = 0; meshIndex < _mesh.GetSubmeshCount(); ++meshIndex)
		{
			_material.Draw(context, uniformBuffer, env, *_modelMatrix, viewProjectionMatrix, _mesh.GetSubmeshInfo(meshIndex).indexCount, _mesh.GetSubmeshInfo(meshIndex).startIndex);
		}
	}

	void SetPosition(const Vector3 &position)
	{
		_position = position;
		_modelMatrix = {};
	}

	void SetRotation(const Quaternion &rotation)
	{
		_rotation = rotation;
		_modelMatrix = {};
	}

private:
	optional<Matrix4x3> _modelMatrix{};
	MeshResource _mesh{};
	RenderingMaterial _material{};
	Vector3 _position{};
	Quaternion _rotation{};
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
			else
			{
				auto meshRenderer = entry.FindComponent<MeshRenderer>();
				if (meshRenderer)
				{
					AddMeshRenderer(env, entry.entityID, *meshRenderer, entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation);
				}
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
		else if (stream.Type() == MeshRenderer::GetTypeId())
		{
			for (auto &entry : stream)
			{
				AddMeshRenderer(env, entry.entityID, entry.added.Cast<MeshRenderer>(), entry.GetComponent<Position>().position, entry.GetComponent<Rotation>().rotation);
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
			MeshRendererPositionChanged(entry.entityID, entry.component);
			CameraPositionChanged(entry.entityID, entry.component);
		}
		for (const auto &entry : stream.Enumerate<Rotation>())
		{
			MeshRendererRotationChanged(entry.entityID, entry.component);
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
			RemoveMeshRenderer(entry.entityID);
			RemoveCamera(entry.entityID);
		}
		for (const auto &entry : stream.Enumerate<Rotation>())
		{
			RemoveMeshRenderer(entry.entityID);
			RemoveCamera(entry.entityID);
		}
		for (const auto &entry : stream.Enumerate<MeshRenderer>())
		{
			RemoveMeshRenderer(entry.entityID);
		}

		ASSUME(stream.Type() == Camera::GetTypeId() || stream.Type() == Position::GetTypeId() || stream.Type() == Rotation::GetTypeId() || stream.Type() == MeshRenderer::GetTypeId());
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
    {
        for (auto id : stream)
        {
			RemoveMeshRenderer(id);
			RemoveCamera(id);
        }
    }
    
    virtual void Update(Environment &env) override
    {
		ASSUME(env.targetSystem == GetTypeId());

        for (auto &camera : _cameras)
        {
			if (camera.data.isClearDepthStencil)
			{
				if (camera.depthStencilBuffer->depthStencilView)
				{
					_context->ClearDepthStencilView(camera.depthStencilBuffer->depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				}
			}

            for (uiw windowIndex = 0; windowIndex < camera.data.rt.size(); ++windowIndex)
            {
                if (auto *window = std::get_if<Window>(&camera.data.rt[windowIndex].target); window)
                {
					auto &dxWindow = camera.windows[windowIndex];
                    if (!dxWindow.hwnd)
                    {
                        new (&dxWindow) DX11Window(*window, camera.depthStencilBuffer, _device.get(), env.logger);
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

						_context->OMSetRenderTargets(1, AddressOfNaked(dxWindow.renderTargetView), camera.depthStencilBuffer->depthStencilView.get());

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

						for (auto &[id, object] : _meshRendererObjects)
						{
							object.Draw(_context.get(), _uniformBuffer.get(), env, viewProjectionMatrix);
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
	void AddMeshRenderer(Environment &env, EntityID id, const MeshRenderer &meshRenderer, const Vector3 &position, const Quaternion &rotation)
	{
		auto insertResult = _meshRendererObjects.insert({id, MeshRendererObject(env, _device.get(), _layoutManager, meshRenderer)});
		ASSUME(insertResult.second);
		insertResult.first->second.SetPosition(position);
		insertResult.first->second.SetRotation(rotation);
	}

	void RemoveMeshRenderer(EntityID id)
	{
		_meshRendererObjects.erase(id);
	}

	void MeshRendererPositionChanged(EntityID id, const Position &newPosition)
	{
		auto findResult = _meshRendererObjects.find(id);
		if (findResult != _meshRendererObjects.end())
		{
			findResult->second.SetPosition(newPosition.position);
		}
	}

	void MeshRendererRotationChanged(EntityID id, const Rotation &newRotation)
	{
		auto findResult = _meshRendererObjects.find(id);
		if (findResult != _meshRendererObjects.end())
		{
			findResult->second.SetRotation(newRotation.rotation);
		}
	}

    void AddCamera(EntityID id, const Camera &data, const CameraTransform &transform)
    {
        _cameras.emplace_front(id, _device.get(), data, transform);
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
	}

private:
	std::unordered_map<EntityID, MeshRendererObject> _meshRendererObjects{};
    std::forward_list<DX11Camera> _cameras{};
    COMUniquePtr<ID3D11Device> _device{};
    COMUniquePtr<ID3D11DeviceContext> _context{};
    D3D_FEATURE_LEVEL _featureLevel{}, _maxSupportedFeatureLevel{};
	COMUniquePtr<ID3D11Buffer> _uniformBuffer{};
	LayoutManager _layoutManager{};
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

DXGI_FORMAT ColorFormatToDirectX(ColorFormatt format)
{
	#define C(colorFormat, dxgiFormat) case ColorFormatt::colorFormat: return dxgiFormat;
	#define U(colorFormat) case ColorFormatt::colorFormat: SOFTBREAK; return DXGI_FORMAT_UNKNOWN;

	switch (format)
	{
		C(Undefined, DXGI_FORMAT_UNKNOWN)
		C(R8G8B8A8, DXGI_FORMAT_R8G8B8A8_TYPELESS)
		C(B8G8R8A8, DXGI_FORMAT_B8G8R8A8_TYPELESS)
		U(R8G8B8)
		U(B8G8R8)
		C(R8G8B8X8, DXGI_FORMAT_R8G8B8A8_TYPELESS)
		C(B8G8R8X8, DXGI_FORMAT_B8G8R8A8_TYPELESS)
		U(R4G4B4A4)
		U(B4G4R4A4)
		U(R5G6B5)
		U(B5G6R5) // supported by DX11.1 and later
		C(R32_Float, DXGI_FORMAT_R32_FLOAT)
		C(R32G32_Float, DXGI_FORMAT_R32G32_FLOAT)
		C(R32G32B32_Float, DXGI_FORMAT_R32G32B32_FLOAT)
		C(R32G32B32A32_Float, DXGI_FORMAT_R32G32B32A32_FLOAT)
		C(R32_Int, DXGI_FORMAT_R32_SINT)
		C(R32G32_Int, DXGI_FORMAT_R32G32_SINT)
		C(R32G32B32_Int, DXGI_FORMAT_R32G32B32_SINT)
		C(R32G32B32A32_Int, DXGI_FORMAT_R32G32B32A32_SINT)
		C(R32_UInt, DXGI_FORMAT_R32_UINT)
		C(R32G32_UInt, DXGI_FORMAT_R32G32_UINT)
		C(R32G32B32_UInt, DXGI_FORMAT_R32G32B32_UINT)
		C(R32G32B32A32_UInt, DXGI_FORMAT_R32G32B32A32_UINT)
		C(R16_Float, DXGI_FORMAT_R16_FLOAT)
		C(R16G16_Float, DXGI_FORMAT_R16G16_FLOAT)
		U(R16G16B16_Float)
		C(R16G16B16A16_Float, DXGI_FORMAT_R16G16B16A16_FLOAT)
		C(R16_Int, DXGI_FORMAT_R16_SINT)
		C(R16G16_Int, DXGI_FORMAT_R16G16_SINT)
		U(R16G16B16_Int)
		C(R16G16B16A16_Int, DXGI_FORMAT_R16G16B16A16_SINT)
		C(R16_UInt, DXGI_FORMAT_R16_UINT)
		C(R16G16_UInt, DXGI_FORMAT_R16G16_UINT)
		U(R16G16B16_UInt)
		C(R16G16B16A16_UInt, DXGI_FORMAT_R16G16B16A16_UINT)
		C(R11G11B10_Float, DXGI_FORMAT_R11G11B10_FLOAT)
		C(D32, DXGI_FORMAT_D32_FLOAT)
		C(D24S8, DXGI_FORMAT_D24_UNORM_S8_UINT)
		C(D24X8, DXGI_FORMAT_D24_UNORM_S8_UINT)
	};

	#undef C
	#undef U

	UNREACHABLE;
	return {};
}

uiw ColorFormatSizeOf(ColorFormatt format)
{
	switch (format)
	{
	case ColorFormatt::Undefined:
		SOFTBREAK;
		return 0;
	case ColorFormatt::R4G4B4A4:
	case ColorFormatt::B4G4R4A4:
	case ColorFormatt::R5G6B5:
	case ColorFormatt::B5G6R5:
	case ColorFormatt::R16_Float:
	case ColorFormatt::R16_Int:
	case ColorFormatt::R16_UInt:
		return 2;
	case ColorFormatt::R8G8B8:
	case ColorFormatt::B8G8R8:
		return 3;
	case ColorFormatt::R8G8B8A8:
	case ColorFormatt::B8G8R8A8:
	case ColorFormatt::R8G8B8X8:
	case ColorFormatt::B8G8R8X8:
	case ColorFormatt::R32_Float:
	case ColorFormatt::R32_Int:
	case ColorFormatt::R32_UInt:
	case ColorFormatt::D32:
	case ColorFormatt::D24S8:
	case ColorFormatt::D24X8:
	case ColorFormatt::R16G16_Float:
	case ColorFormatt::R16G16_Int:
	case ColorFormatt::R16G16_UInt:
	case ColorFormatt::R11G11B10_Float:
		return 4;
	case ColorFormatt::R16G16B16_Float:
	case ColorFormatt::R16G16B16_Int:
	case ColorFormatt::R16G16B16_UInt:
		return 6;
	case ColorFormatt::R32G32_Float:
	case ColorFormatt::R32G32_Int:
	case ColorFormatt::R32G32_UInt:
	case ColorFormatt::R16G16B16A16_Float:
	case ColorFormatt::R16G16B16A16_Int:
	case ColorFormatt::R16G16B16A16_UInt:
		return 8;
	case ColorFormatt::R32G32B32_Float:
	case ColorFormatt::R32G32B32_Int:
	case ColorFormatt::R32G32B32_UInt:
		return 12;
	case ColorFormatt::R32G32B32A32_Float:
	case ColorFormatt::R32G32B32A32_Int:
	case ColorFormatt::R32G32B32A32_UInt:
		return 16;
	}

	UNREACHABLE;
	return {};
}

DX11DepthStencilBuffer::DX11DepthStencilBuffer(ID3D11Device *device, Camera::DepthBufferFormat depthBufferFormat) : device(device), depthBufferFormat(depthBufferFormat)
{}

bool DX11DepthStencilBuffer::Resize(LoggerWrapper &logger, ui32 newWidth, ui32 newHeight)
{
	if (depthBufferFormat == Camera::DepthBufferFormat::Neither)
	{
		ASSUME(!depthStencilView);
		return true;
	}

	if (newWidth == width && newHeight == height)
	{
		return true;
	}

	width = newWidth;
	height = newHeight;

	DXGI_FORMAT format;
	if (depthBufferFormat == Camera::DepthBufferFormat::DepthOnly)
	{
		format = DXGI_FORMAT_D32_FLOAT;
	}
	else
	{
		format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	}

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = format;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	COMUniquePtr<ID3D11Texture2D> depthStencilTexture;
	if (HRESULT hresult = device->CreateTexture2D(&depthStencilDesc, 0, AddressOfNaked(depthStencilTexture)); hresult != S_OK)
	{
		logger.Error("CreateWindowRenderTargetView: create depth stencil texture for the current window failed with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
		return false;
	}

	if (HRESULT hresult = device->CreateDepthStencilView(depthStencilTexture.get(), 0, AddressOfNaked(depthStencilView)); hresult != S_OK)
	{
		logger.Error("CreateWindowRenderTargetView: create depth stencil view for the current window failed with error 0x%Xl %s\n", hresult, ConvertDirectXErrToString(hresult));
		return false;
	}

	return true;
}

DX11Window::DX11Window(DX11Window &&source) noexcept : 
    isChanged(source.isChanged),
    window(source.window),
    hwnd(move(source.hwnd)),
    controlsQueue(move(source.controlsQueue)),
    hidInput(move(source.hidInput)),
    vkInput(move(source.vkInput)),
    swapChain(source.swapChain.release()), 
    renderTargetView(source.renderTargetView.release()),
	depthStencilBuffer(source.depthStencilBuffer)
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
	depthStencilBuffer = move(source.depthStencilBuffer);

	return *this;
}

DX11Window::DX11Window(Window &window, const shared_ptr<DX11DepthStencilBuffer> &depthStencilBuffer, ID3D11Device *device, LoggerWrapper &logger) : window(&window), depthStencilBuffer(depthStencilBuffer), device(device)
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

    if (HRESULT hresult = swapChain->ResizeBuffers(0, window->width, window->height, DXGI_FORMAT_UNKNOWN, 0); hresult != S_OK)
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

	if (!depthStencilBuffer->Resize(logger, window->width, window->height))
	{
		logger.Error("CreateWindowRenderTargetView: failed to resuze depth stencil buffer\n");
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

DX11Camera::DX11Camera(EntityID id, ID3D11Device *device, const Camera &data, const CameraTransform &transform) : id(id), data(data), transform(transform), depthStencilBuffer(make_shared<DX11DepthStencilBuffer>(device, data.depthBufferFormat))
{
}