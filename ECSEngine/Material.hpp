#pragma once

#include "Texture.hpp"
#include "Color.hpp"

namespace ECSEngine
{
	enum class PrimitiveTopologyt : ui8
	{
		TriangleEnumeration,
		Point,
		TriangleStrip,
		TriangleFan
	};

	enum class PolygonFillModet : ui8
	{
		Solid,
		Wireframe
	};

	enum class PolygonCullModet : ui8
	{
		Back,
		Front,
		None
	};

	enum class DepthComparisonFunct : ui8
	{
		Never,
		Less,
		LessEqual,
		Equal,
		NotEqual,
		GreaterEqual,
		Greater,
		Always
	};

	enum class BlendFactort : ui8
	{
		Zero,
		One,
		SourceColor,
		InvertSourceColor,
		SourceAlpha,
		InvertSourceAlpha,
		TargetColor,
		InvertTargetColor,
		TargetAlpha,
		InvertTargetAlpha
	};

	enum class BlendCombineModet : ui8
	{
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max
	};

	struct Shader
	{
		struct Uniform
		{
			string name{};
			variant<std::monostate, Matrix4x4, Matrix4x3, Matrix3x4, Matrix3x3, Matrix3x2, Matrix2x3, Matrix2x2, ColorR8G8B8A8, i32, ui32, bool, Vector4, Vector3, Vector2, f32, TextureAsset> data{};
		};

		string name{};
		vector<Uniform> uniforms{};
	};

	struct BlendStates
	{
		bool isBlendingEnabled{};
		BlendFactort blendingSourceColorFactor{};
		BlendFactort blendingSourceAlphaFactor{};
		BlendFactort blendingTargetColorFactor{};
		BlendFactort blendingTargetAlphaFactor{};
		BlendCombineModet blendingColorCombineMode{};
		BlendCombineModet blendingAlphaCombineMode{};
	};

	struct Material
	{
		Shader shader{};
		PolygonCullModet polygonCullMode{};
		PrimitiveTopologyt primitiveTopology{};
		PolygonFillModet polygonFillMode{};
		DepthComparisonFunct depthComparisonFunc{};
		bool isPolygonFrontCounterClockwise{};
		bool isDepthWriteEnabled{};
		BlendStates blendStates[8]{};
		// TODO: stencil
		// TODO: scissors
	};

	struct MaterialAssetId : AssetId
	{
		using AssetId::AssetId;
	};

	struct MaterialAsset
	{
		using assetIdType = MaterialAssetId;
		assetIdType assetId{};
		Material material{};
	};

	struct MaterialAssetsArrayAssetId : AssetId
	{
		using AssetId::AssetId;
	};

	struct MaterialAssetsArrayAsset
	{
		using assetIdType = MaterialAssetsArrayAssetId;
		assetIdType assetId{};
		vector<MaterialAsset> materialAssets{};
	};
}