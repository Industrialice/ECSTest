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

	struct MeshAsset : TypeIdentifiable<MeshAsset>
	{
		using assetIdType = MeshAssetId;
		assetIdType assetId{};
		Mesh desc{};
		unique_ptr<byte[]> data{};
	};
}

namespace std
{
	template <> struct hash<ECSEngine::MeshAssetId>
	{
		[[nodiscard]] size_t operator()(const ECSEngine::MeshAssetId &value) const
		{
			return value.Hash();
		}
	};
}