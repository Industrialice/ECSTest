#include "PreHeader.hpp"
#include "AssetsLoaders.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "AssetsIdentification.hpp"

using namespace ECSEngine;
using namespace std::placeholders;

static optional<MeshAsset> LoadProcedural(Array<const byte> source, uiw subMesh, f32 globalScale);
static optional<MeshAsset> LoadWithAssimp(Array<const byte> source, uiw subMesh, f32 globalScale, bool isUseFileScale);

AssetsManager::AssetLoaderFuncType AssetsLoaders::GenerateMeshLoaderFunction()
{
	return std::bind(&AssetsLoaders::LoadMesh, this, _1, _2);
}

AssetsManager::AssetLoaderFuncType AssetsLoaders::GenerateTextureLoaderFunction()
{
	return std::bind(&AssetsLoaders::LoadTexture, this, _1, _2);
}

AssetsManager::AssetLoaderFuncType AssetsLoaders::GeneratePhysicsPropertiesLoaderFunction()
{
	return std::bind(&AssetsLoaders::LoadPhysicsProperties, this, _1, _2);
}

void AssetsLoaders::RegisterLoaders(AssetsManager &manager)
{
	manager.AddAssetLoader(MeshAsset::GetTypeId(), GenerateMeshLoaderFunction());
	manager.AddAssetLoader(TextureAsset::GetTypeId(), GenerateTextureLoaderFunction());
	manager.AddAssetLoader(PhysicsPropertiesAsset::GetTypeId(), GeneratePhysicsPropertiesLoaderFunction());
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

	auto *identifier = _assetIdMapper->Resolve(id);
	if (!identifier)
	{
		SOFTBREAK;
		return {};
	}

	if (identifier->AssetTypeId() != MeshAsset::GetTypeId())
	{
		SOFTBREAK;
		return {};
	}

	auto *meshIdentifier = static_cast<const MeshPathAssetIdentification *>(identifier);

	Error<> fileError;
	File file(meshIdentifier->AssetFilePath(), FileOpenMode::OpenExisting, FileProcModes::Read, 0, FileCacheModes::LinearRead, FileShareModes::Read, &fileError);
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

	if (meshIdentifier->AssetFilePath().ExtensionView() == TSTR("procedural"))
	{
		auto loaded = LoadProcedural(Array(mapping.CMemory(), mapping.Size()), meshIdentifier->SubMeshIndex(), meshIdentifier->GlobalScale());
		if (!loaded)
		{
			SOFTBREAK;
			return {};
		}
		return {MeshAsset::GetTypeId(), make_shared<MeshAsset>(move(*loaded))};
	}
	else
	{
		auto loaded = LoadWithAssimp(Array(mapping.CMemory(), mapping.Size()), meshIdentifier->SubMeshIndex(), meshIdentifier->GlobalScale(), meshIdentifier->IsUseFileScale());
		if (!loaded)
		{
			SOFTBREAK;
			return {};
		}
		return {MeshAsset::GetTypeId(), make_shared<MeshAsset>(move(*loaded))};
	}
}

AssetsManager::LoadedAsset AssetsLoaders::LoadTexture(AssetId id, TypeId expectedType)
{
	NOIMPL;
	return {};
}

AssetsManager::LoadedAsset AssetsLoaders::LoadPhysicsProperties(AssetId id, TypeId expectedType)
{
	if (!_assetIdMapper)
	{
		SOFTBREAK;
		return {};
	}

	auto *identifier = _assetIdMapper->Resolve(id);
	if (!identifier)
	{
		SOFTBREAK;
		return {};
	}

	if (identifier->AssetTypeId() != PhysicsPropertiesAsset::GetTypeId())
	{
		SOFTBREAK;
		return {};
	}

	auto *properties = static_cast<const PhysicsPropertiesAssetIdentification *>(identifier);

	return {PhysicsPropertiesAsset::GetTypeId(), make_shared<PhysicsProperties>(properties->Properties())};
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

		auto AssimpMatrixToStdLib = [](const aiMatrix4x4 &matrix)
		{
			Matrix4x3 transformation = {
				matrix.a1, matrix.b1, matrix.c1,
				matrix.a2, matrix.b2, matrix.c2,
				matrix.a3, matrix.b3, matrix.c3,
				matrix.a4, matrix.b4, matrix.c4};
			return transformation;
		};

		Matrix4x3 transformation = AssimpMatrixToStdLib(node->mTransformation);

		const aiNode *parent = node->mParent;
		while (parent)
		{
			transformation *= AssimpMatrixToStdLib(parent->mTransformation);
			parent = parent->mParent;
		}

		results.push_back({mesh, transformation});
	}

	for (uiw childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
	{
		auto childMeshes = GetMeshesFromHierarchy(scene, node->mChildren[childIndex]);
		results.insert(results.end(), childMeshes.begin(), childMeshes.end());
	}

	return results;
}

optional<MeshAsset> LoadProcedural(Array<const byte> source, uiw subMesh, f32 globalScale)
{
	MeshAsset loaded{};
	const byte *ptr = source.data();

	ui32 verticesCount, indexesCount;
	MemOps::Copy(&verticesCount, reinterpret_cast<const ui32 *>(ptr), 1);
	MemOps::Copy(&indexesCount, reinterpret_cast<const ui32 *>(ptr + sizeof(ui32)), 1);
	ptr += sizeof(ui32) * 2;

	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
	};

	const byte *verticesStart = ptr;
	ptr += verticesCount * sizeof(Vertex);

	vector<ui32> indexesTemp(indexesCount);
	MemOps::Copy(indexesTemp.data(), reinterpret_cast<const ui32 *>(ptr), indexesCount);

	vector<ui16> indexes(indexesCount);
	for (uiw index = 0; index < indexesCount; ++index)
	{
		ASSUME(indexesTemp[index] <= ui16_max);
		indexes[index] = static_cast<ui16>(indexesTemp[index]);
	}

	loaded.desc.subMeshInfos.push_back({.vertexCount = verticesCount,.indexCount = indexesCount,.transformation = {}});

	if (verticesCount == 0 || indexesCount == 0)
	{
		SOFTBREAK;
		return {};
	}

	uiw verticesSize = verticesCount * sizeof(Vertex);
	uiw indexesSize = indexesCount * sizeof(ui16);

	loaded.data = make_unique<byte[]>(verticesSize + indexesSize);
	MemOps::Copy(loaded.data.get(), verticesStart, verticesSize);
	MemOps::Copy(loaded.data.get() + verticesSize, reinterpret_cast<const byte *>(indexes.data()), indexesSize);

	loaded.desc.vertexAttributes.push_back({"position", ColorFormatt::R32G32B32_Float});
	loaded.desc.vertexAttributes.push_back({"normal", ColorFormatt::R32G32B32_Float});

	return loaded;
}

optional<MeshAsset> LoadWithAssimp(Array<const byte> source, uiw subMesh, f32 globalScale, bool isUseFileScale)
{
	MeshAsset loaded{};
	Assimp::Importer importer;
	int flags = aiProcess_Triangulate;
	flags |= aiProcess_SortByPType;
	flags |= aiProcess_JoinIdenticalVertices;
	flags |= aiProcess_FindDegenerates;
	flags |= aiProcess_FindInvalidData;
	//flags |= aiProcess_PreTransformVertices;
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
	if (isUseFileScale && scene->mMetaData && scene->mMetaData->Get("UnitScaleFactor", scaleFactor))
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

	constexpr f32 scaleAdjust = 0.01f; // Assimp assumes FBX uses cm
	f32 vertexScale = globalScale * scaleAdjust * scale32;

	meshes[subMesh].transformation.SetRow(3, meshes[subMesh].transformation.GetRow(3) *vertexScale);

	ui16 vertexCount = static_cast<ui16>(mesh->mNumVertices);
	ui32 indexCount = mesh->mNumFaces * 3;

	vertices.resize(totalVertices + vertexCount);

	//File testOutput(L"C:\\Users\\salal\\Desktop\\assimp.txt", FileOpenMode::CreateAlways, FileProcModes::Write);

	for (uiw vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
	{
		aiVector3D position = mesh->mVertices[vertexIndex];
		aiVector3D normal = mesh->mNormals[vertexIndex];

		vertices[totalVertices + vertexIndex] = {.position = Vector3{-position.x, position.y, position.z} *vertexScale,.normal = {-normal.x, normal.y, normal.z}};

		//Vector3 pos = vertices[totalVertices + vertexIndex].position;
		//testOutput.WriteFormatted("%.3f,%.3f,%.3f\n", pos.x, pos.y, pos.z);
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