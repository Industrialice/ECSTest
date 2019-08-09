#include "PreHeader.hpp"
#include "AssetsLoaders.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

using namespace ECSEngine;
using namespace std::placeholders;

AssetsManager::AssetLoaderFuncType AssetsLoaders::GenerateMeshLoaderFunction()
{
	return std::bind(&AssetsLoaders::LoadMesh, this, _1, _2);
}

AssetsManager::AssetLoaderFuncType AssetsLoaders::GenerateTextureLoaderFunction()
{
	return std::bind(&AssetsLoaders::LoadTexture, this, _1, _2);
}

void AssetsLoaders::RegisterLoaders(AssetsManager &manager)
{
	manager.AddAssetLoader(MeshAsset::GetTypeId(), GenerateMeshLoaderFunction());
	manager.AddAssetLoader(TextureAsset::GetTypeId(), GenerateTextureLoaderFunction());
}

AssetsManager::LoadedAsset AssetsLoaders::LoadMesh(AssetId id, TypeId expectedType)
{
	if (expectedType != MeshAsset::GetTypeId())
	{
		SOFTBREAK;
		return {MeshAsset::GetTypeId()};
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

	auto data = make_unique<byte[]>(sizeof(vertexArrayData) + sizeof(indexes));
	MemOps::Copy(data.get(), reinterpret_cast<const byte *>(vertexArrayData), sizeof(vertexArrayData));
	MemOps::Copy(data.get() + sizeof(vertexArrayData), reinterpret_cast<const byte *>(indexes), sizeof(indexes));

	Mesh::SubMeshInfo subMesh;
	subMesh.vertexCount = CountOf(vertexArrayData);
	subMesh.indexCount = 36;

	Mesh::VertexAttribute vertexAttributes[] =
	{
		{"POSITION", ColorFormatt::R32G32B32_Float},
		{"COLOR", ColorFormatt::R32G32B32A32_Float}
	};

	Mesh mesh;
	mesh.isSkinned = false;
	mesh.subMeshInfos.push_back(subMesh);
	mesh.vertexAttributes.assign(std::begin(vertexAttributes), std::end(vertexAttributes));

	MeshAsset meshAsset;
	meshAsset.assetId = {};
	meshAsset.desc = mesh;
	meshAsset.data = move(data);

	return {MeshAsset::GetTypeId(), make_shared<MeshAsset>(move(meshAsset))};
}

AssetsManager::LoadedAsset AssetsLoaders::LoadTexture(AssetId id, TypeId expectedType)
{
	NOIMPL;
	return {};
}