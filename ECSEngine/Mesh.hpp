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
			Matrix4x3 transformation{};
		};

		struct VertexAttribute
		{
			string name{};
			ColorFormatt type{};
		};

		bool isSkinned{};
		vector<SubMeshInfo> subMeshInfos{};
		vector<VertexAttribute> vertexAttributes{};

		uiw Stride() const
		{
			uiw stride = 0;
			for (const auto &attr : vertexAttributes)
			{
				stride += ColorFormatSizeOf(attr.type);
			}
			return stride;
		}

		uiw TotalVertexCount() const
		{
			uiw total = 0;
			for (const auto &info : subMeshInfos)
			{
				total += info.vertexCount;
			}
			return total;
		}

		uiw TotalIndexCount() const
		{
			uiw total = 0;
			for (const auto &info : subMeshInfos)
			{
				total += info.indexCount;
			}
			return total;
		}
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