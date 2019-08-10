#include "PreHeader.hpp"
#include "AssetsLoaders.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

using namespace ECSEngine;
using namespace std::placeholders;

static optional<MeshAsset> LoadWithAssimp(Array<const byte> source);

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

	File file(L"Assets/Raider_Ponyhide_Armor_female.fbx", FileOpenMode::OpenExisting, FileProcModes::Read);
	if (!file)
	{
		SOFTBREAK;
		return {};
	}

	MemoryMappedFile mapping(file);
	if (!mapping)
	{
		SOFTBREAK;
		return {};
	}

	auto loaded = LoadWithAssimp(Array(mapping.CMemory(), mapping.Size()));
	if (!loaded)
	{
		SOFTBREAK;
		return {};
	}

	//static constexpr f32 transparency = 0.5f;

	//static constexpr Vector4 frontColor{1, 1, 1, transparency};
	//static constexpr Vector4 upColor{0, 1, 1, transparency};
	//static constexpr Vector4 backColor{0, 0, 1, transparency};
	//static constexpr Vector4 downColor{1, 0, 0, transparency};
	//static constexpr Vector4 leftColor{1, 1, 0, transparency};
	//static constexpr Vector4 rightColor{1, 0, 1, transparency};

	//struct Vertex
	//{
	//	Vector3 position;
	//	Vector4 color;
	//};

	//static constexpr Vertex vertexArrayData[]
	//{
	//	// front
	//	{{-0.5f, -0.5f, -0.5f}, frontColor},
	//	{{-0.5f, 0.5f, -0.5f}, frontColor},
	//	{{0.5f, -0.5f, -0.5f}, frontColor},
	//	{{0.5f, 0.5f, -0.5f}, frontColor},

	//	// up
	//	{{-0.5f, 0.5f, -0.5f}, upColor},
	//	{{-0.5f, 0.5f, 0.5f}, upColor},
	//	{{0.5f, 0.5f, -0.5f}, upColor},
	//	{{0.5f, 0.5f, 0.5f}, upColor},

	//	// back
	//	{{0.5f, -0.5f, 0.5f}, backColor},
	//	{{0.5f, 0.5f, 0.5f}, backColor},
	//	{{-0.5f, -0.5f, 0.5f}, backColor},
	//	{{-0.5f, 0.5f, 0.5f}, backColor},

	//	// down
	//	{{-0.5f, -0.5f, 0.5f}, downColor},
	//	{{-0.5f, -0.5f, -0.5f}, downColor},
	//	{{0.5f, -0.5f, 0.5f}, downColor},
	//	{{0.5f, -0.5f, -0.5f}, downColor},

	//	// left
	//	{{-0.5f, -0.5f, 0.5f}, leftColor},
	//	{{-0.5f, 0.5f, 0.5f}, leftColor},
	//	{{-0.5f, -0.5f, -0.5f}, leftColor},
	//	{{-0.5f, 0.5f, -0.5f}, leftColor},

	//	// right
	//	{{0.5f, -0.5f, -0.5f}, rightColor},
	//	{{0.5f, 0.5f, -0.5f}, rightColor},
	//	{{0.5f, -0.5f, 0.5f}, rightColor},
	//	{{0.5f, 0.5f, 0.5f}, rightColor},
	//};

	//ui16 indexes[36];
	//for (ui32 index = 0; index < 6; ++index)
	//{
	//	indexes[index * 6 + 0] = index * 4 + 0;
	//	indexes[index * 6 + 1] = index * 4 + 1;
	//	indexes[index * 6 + 2] = index * 4 + 3;

	//	indexes[index * 6 + 3] = index * 4 + 2;
	//	indexes[index * 6 + 4] = index * 4 + 0;
	//	indexes[index * 6 + 5] = index * 4 + 3;
	//}

	//auto data = make_unique<byte[]>(sizeof(vertexArrayData) + sizeof(indexes));
	//MemOps::Copy(data.get(), reinterpret_cast<const byte *>(vertexArrayData), sizeof(vertexArrayData));
	//MemOps::Copy(data.get() + sizeof(vertexArrayData), reinterpret_cast<const byte *>(indexes), sizeof(indexes));

	//Mesh::SubMeshInfo subMesh;
	//subMesh.vertexCount = CountOf(vertexArrayData);
	//subMesh.indexCount = 36;

	//Mesh::VertexAttribute vertexAttributes[] =
	//{
	//	{"POSITION", ColorFormatt::R32G32B32_Float},
	//	{"COLOR", ColorFormatt::R32G32B32A32_Float}
	//};

	//Mesh mesh;
	//mesh.isSkinned = false;
	//mesh.subMeshInfos.push_back(subMesh);
	//mesh.vertexAttributes.assign(std::begin(vertexAttributes), std::end(vertexAttributes));

	//MeshAsset meshAsset;
	//meshAsset.assetId = {};
	//meshAsset.desc = mesh;
	//meshAsset.data = move(data);

	return {MeshAsset::GetTypeId(), make_shared<MeshAsset>(move(*loaded))};
}

AssetsManager::LoadedAsset AssetsLoaders::LoadTexture(AssetId id, TypeId expectedType)
{
	NOIMPL;
	return {};
}

optional<MeshAsset> LoadWithAssimp(Array<const byte> source)
{
	MeshAsset loaded{};
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFileFromMemory(source.data(), source.size(), aiProcess_Triangulate | aiProcess_SortByPType);
	if (!scene)
	{
		SENDLOG(Error, AssetsLoaders, "LoadWithAssimp failed to read file from memory\n");
		return {};
	}

	struct Vertex
	{
		Vector3 position;
		Vector4 color;
	};

	vector<Vertex> vertices;
	vector<ui16> indexes;

	ui16 totalVertices = 0;
	ui32 totalIndexes = 0;

	for (uiw meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		const aiMesh *mesh = scene->mMeshes[meshIndex];

		ASSUME(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

		ui16 vertexCount = static_cast<ui16>(mesh->mNumVertices);
		ui32 indexCount = mesh->mNumFaces * 3;

		vertices.resize(totalVertices + vertexCount);

		for (uiw vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
		{
			aiVector3D position = mesh->mVertices[vertexIndex];

			Vector4 color;
			color.x = rand() / static_cast<f32>(RAND_MAX);
			color.y = rand() / static_cast<f32>(RAND_MAX);
			color.z = rand() / static_cast<f32>(RAND_MAX);
			color.w = rand() / static_cast<f32>(RAND_MAX);

			vertices[totalVertices + vertexIndex] = {.position = {position.x, position.y, position.z},.color = color};
		}

		indexes.resize(totalIndexes + mesh->mNumFaces * 3);
		for (uiw faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			ASSUME(mesh->mFaces[faceIndex].mNumIndices == 3);
			for (uiw subIndex = 0; subIndex < 3; ++subIndex)
			{
				indexes[totalIndexes + faceIndex * 3 + subIndex] = mesh->mFaces[faceIndex].mIndices[subIndex] + totalVertices;
			}
		}

		totalVertices += vertexCount;
		totalIndexes += indexCount;

		loaded.desc.subMeshInfos.push_back({.vertexCount = vertexCount,.indexCount = indexCount});
	}

	uiw verticesSize = totalVertices * sizeof(Vertex);
	uiw indexesSize = totalIndexes * sizeof(ui16);

	loaded.data = make_unique<byte[]>(verticesSize + indexesSize);
	MemOps::Copy(loaded.data.get(), reinterpret_cast<const byte *>(vertices.data()), verticesSize);
	MemOps::Copy(loaded.data.get() + verticesSize, reinterpret_cast<const byte *>(indexes.data()), indexesSize);

	loaded.desc.vertexAttributes.push_back({"position", ColorFormatt::R32G32B32_Float});
	loaded.desc.vertexAttributes.push_back({"color", ColorFormatt::R32G32B32A32_Float});

	return loaded;
}