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

void AssetsLoaders::SetAssetIdMapper(const shared_ptr<AssetIdMapper> &mapper)
{
	_assetIdMapper = mapper;
}

void AssetsLoaders::SetAssetsLocation(FilePath &&path)
{
	_assetsLocation = move(path);
}

AssetsManager::LoadedAsset AssetsLoaders::LoadMesh(AssetId id, TypeId expectedType)
{
	if (expectedType != MeshAsset::GetTypeId())
	{
		SOFTBREAK;
		return {MeshAsset::GetTypeId()};
	}

	if (!_assetIdMapper)
	{
		SOFTBREAK;
		return {};
	}

	auto filePathAndType = _assetIdMapper->ResolveIdToPath(id);
	if (!filePathAndType)
	{
		SOFTBREAK;
		return {};
	}

	if (filePathAndType->second != MeshAsset::GetTypeId())
	{
		SOFTBREAK;
		return {};
	}

	Error<> fileError;
	FilePath fullPath;
	if (_assetsLocation.IsEmpty())
	{
		fullPath = filePathAndType->first;
	}
	else
	{
		fullPath = _assetsLocation / filePathAndType->first;
	}
	File file(_assetsLocation / filePathAndType->first, FileOpenMode::OpenExisting, FileProcModes::Read, 0, {}, {}, &fileError);
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
	constexpr auto flags = aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices | aiProcess_FindDegenerates | aiProcess_FindInvalidData | aiProcess_MakeLeftHanded;
	const aiScene *scene = importer.ReadFileFromMemory(source.data(), source.size(), flags);
	if (!scene)
	{
		SENDLOG(Error, AssetsLoaders, "LoadWithAssimp failed to read file from memory\n");
		return {};
	}

	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
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

		if (!mesh->HasNormals())
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
			aiVector3D normal = mesh->mNormals[vertexIndex];

			vertices[totalVertices + vertexIndex] = {.position = {position.x, position.y, position.z},.normal = {normal.x, normal.y, normal.z}};
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

	if (totalVertices == 0 || totalIndexes == 0)
	{
		SOFTBREAK;
		return {};
	}

	uiw verticesSize = totalVertices * sizeof(Vertex);
	uiw indexesSize = totalIndexes * sizeof(ui16);

	loaded.data = make_unique<byte[]>(verticesSize + indexesSize);
	MemOps::Copy(loaded.data.get(), reinterpret_cast<const byte *>(vertices.data()), verticesSize);
	MemOps::Copy(loaded.data.get() + verticesSize, reinterpret_cast<const byte *>(indexes.data()), indexesSize);

	loaded.desc.vertexAttributes.push_back({"position", ColorFormatt::R32G32B32_Float});
	loaded.desc.vertexAttributes.push_back({"normal", ColorFormatt::R32G32B32_Float});

	return loaded;
}