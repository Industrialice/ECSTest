#include "PreHeader.hpp"
#include "AssetsLoaders.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

using namespace ECSEngine;
using namespace std::placeholders;

static optional<MeshAsset> LoadWithAssimp(Array<const byte> source, uiw subMesh);

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

	uiw subMeshIndex = 0;

	auto platformPath = filePathAndType->first.PlatformPath();
	uiw commaIndex = platformPath.size() - 1;
	for (; commaIndex != uiw_max; --commaIndex)
	{
		if (platformPath[commaIndex] == ',')
		{
			break;
		}
	}

	if (commaIndex == uiw_max)
	{
		SOFTBREAK; // didn't find submesh index
		return {};
	}

	std::array<char, 32> temp;
	const wchar_t *start = platformPath.data() + commaIndex + 1;
	const wchar_t *end = platformPath.data() + platformPath.size();
	uiw index = 0;
	for (; index < 32 && &start[index] != end; ++index)
	{
		temp[index] = static_cast<char>(start[index]);
	}
	if (auto [ptr, error] = std::from_chars(temp.data(), temp.data() + index, subMeshIndex); error != std::errc())
	{
		SOFTBREAK;
	}

	platformPath = platformPath.substr(0, commaIndex);

	Error<> fileError;
	File file(platformPath, FileOpenMode::OpenExisting, FileProcModes::Read, 0, {}, FileShareModes::Read, &fileError);
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

	auto loaded = LoadWithAssimp(Array(mapping.CMemory(), mapping.Size()), subMeshIndex);
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

struct StoredMesh
{
	const aiMesh *mesh{};
	Matrix4x3 transformation{};
};

static vector<StoredMesh> GetMeshesFromHierarchy(const aiScene *scene, const aiNode *node)
{
	vector<StoredMesh> results;

	for (uiw index = 0; index < node->mNumMeshes; ++index)
	{
		uiw meshInternalIndex = node->mMeshes[index];
		const aiMesh *mesh = scene->mMeshes[meshInternalIndex];
		if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
		{
			continue;
		}
		
		Matrix4x3 transformation = {
			node->mTransformation.a1, node->mTransformation.b1, node->mTransformation.c1,
			node->mTransformation.a2, node->mTransformation.b2, node->mTransformation.c2,
			node->mTransformation.a3, node->mTransformation.b3, node->mTransformation.c3,
			node->mTransformation.a4, node->mTransformation.b4, node->mTransformation.c4};

		results.push_back({mesh, transformation});
	}

	for (uiw childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
	{
		auto childMeshes = GetMeshesFromHierarchy(scene, node->mChildren[childIndex]);
		results.insert(results.end(), childMeshes.begin(), childMeshes.end());
	}

	return results;
}

optional<MeshAsset> LoadWithAssimp(Array<const byte> source, uiw subMesh)
{
	MeshAsset loaded{};
	Assimp::Importer importer;
	auto flags = aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_JoinIdenticalVertices | /*aiProcess_PreTransformVertices | */aiProcess_FindDegenerates | aiProcess_FindInvalidData;
	//flags |= aiProcess_ConvertToLeftHanded;
	flags |= aiProcess_FlipWindingOrder;

	//importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 0.01f); // assimp’s FBX importer assuming scale is in centimeters
	//flags |= aiProcess_GlobalScale;

	if (!importer.ValidateFlags(flags))
	{
		SOFTBREAK;
		return {};
	}

	const aiScene *scene = importer.ReadFileFromMemory(source.data(), source.size(), flags);
	if (!scene)
	{
		SENDLOG(Error, AssetsLoaders, "LoadWithAssimp failed to read file from memory\n");
		return {};
	}

	f64 scaleFactor = 1.0;
	f32 scale32 = 1.0f;
	if (scene->mMetaData && scene->mMetaData->Get("UnitScaleFactor", scaleFactor))
	{
		scale32 = static_cast<f32>(scaleFactor);
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

	vector<StoredMesh> meshes = GetMeshesFromHierarchy(scene, scene->mRootNode);

	if (subMesh >= meshes.size())
	{
		SOFTBREAK;
		return {};
	}

	const aiMesh *mesh = meshes[subMesh].mesh;

	if (!mesh->HasNormals())
	{
		SOFTBREAK;
		return {};
	}

	ui16 vertexCount = static_cast<ui16>(mesh->mNumVertices);
	ui32 indexCount = mesh->mNumFaces * 3;

	vertices.resize(totalVertices + vertexCount);

	for (uiw vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
	{
		aiVector3D position = mesh->mVertices[vertexIndex];
		aiVector3D normal = mesh->mNormals[vertexIndex];

		vertices[totalVertices + vertexIndex] = {.position = Vector3{-position.x, position.y, position.z} * 0.01f * scale32,.normal = {-normal.x, normal.y, normal.z}};
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

	loaded.desc.subMeshInfos.push_back({.vertexCount = vertexCount,.indexCount = indexCount,.transformation = meshes[subMesh].transformation});

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