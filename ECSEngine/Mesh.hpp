#pragma once

#include "Texture.hpp"

namespace ECSEngine
{
	struct Mesh
	{
		struct SubMeshInfo
		{
			ui32 vertexCount{};
			ui32 indexCount{};
		};

		struct VertexAttribute
		{
			string name{};
			ColorFormatt type{};
		};

		bool isSkinned{};
		vector<SubMeshInfo> subMeshInfos{};
		vector<VertexAttribute> vertexAttributes{};
	};

	struct MeshAssetId : AssetId
	{
		using AssetId::AssetId;
	};

	struct MeshAsset
	{
		MeshAssetId assetId{};
		Mesh desc{};
		unique_ptr<byte[]> data{};
	};
}