#pragma once

namespace ECSEngine
{
	enum class WrapModet : ui8
	{
		Tile,     // aka Wrap or Repeat
		Mirror,
		Mirror_once,  // mirror once, the repeat the closes color to the border
		Border,       // fetch border color if out of range
		ClampToBorder // aka ClampToEdge, repeat the closest color to the border
	};

	enum class FilterModet : ui8
	{
		Nearest,
		Linear,
		Anisotropic
	};

	enum class TextureDimensiont : ui8
	{
		Undefined,
		Tex2D,
		Tex3D,
		TexCube
	};

	// TODO: rename into something more appropriate
	enum class ColorFormatt : ui8
	{
		Undefined,
		R8G8B8A8,
		B8G8R8A8,
		R8G8B8,
		B8G8R8,
		R8G8B8X8,
		B8G8R8X8,
		R4G4B4A4,
		B4G4R4A4,
		R5G6B5,
		B5G6R5,
		R32_Float,
		R32G32_Float,
		R32G32B32_Float,
		R32G32B32A32_Float,
		R32_Int,
		R32G32_Int,
		R32G32B32_Int,
		R32G32B32A32_Int,
		R32_UInt,
		R32G32_UInt,
		R32G32B32_UInt,
		R32G32B32A32_UInt,
		R16_Float,
		R16G16_Float,
		R16G16B16_Float,
		R16G16B16A16_Float,
		R16_Int,
		R16G16_Int,
		R16G16B16_Int,
		R16G16B16A16_Int,
		R16_UInt,
		R16G16_UInt,
		R16G16B16_UInt,
		R16G16B16A16_UInt,
		R11G11B10_Float,
		D32,
		D24S8,
		D24X8
	};

	struct CPUAccessMode
	{
		enum class Modet : ui8
		{
			NotAllowed,
			RareFull,
			RarePartial,
			FrequentFull,
			FrequentPartial
		};

		Modet writeMode{};
		Modet readMode{};
	};

	struct GPUAccessMode
	{
		bool isAllowUnorderedWrite;
		bool isAllowUnorderedRead;
	};

	struct Texture
	{
		ui16 width{};
		ui16 height{};
		ui16 depth{};
		TextureDimensiont dimension{};
		ColorFormatt colorFormat{};
		ui8 mipLevelsCount{};
		WrapModet xWrapMode{};
		WrapModet yWrapMode{};
		WrapModet zWrapMode{};
		FilterModet minifyFilterMode{};
		FilterModet magnifyFilterMode{};
		f32 borderColor[4]{};
		bool isMinifyInterpolatesBetweenMips{};
		ui8 anisotropyLevel{};
		CPUAccessMode cpuMode{};
		GPUAccessMode gpuMode{};
	};

	struct TextureAssetId : public AssetId
	{
		using AssetId::AssetId;
	};

	struct TextureAsset : TypeIdentifiable<TextureAsset>
	{
		TextureAssetId assetId{};
		Texture desc{};
		unique_ptr<byte> data{};
	};
}