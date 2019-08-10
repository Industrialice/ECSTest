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

		if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
		{
			SOFTBREAK;
			continue;
		}

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