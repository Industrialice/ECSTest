#include "PreHeader.hpp"
#include "AssetsIdentification.hpp"
#include "Mesh.hpp"

using namespace ECSEngine;

shared_ptr<MeshPathAssetIdentification> MeshPathAssetIdentification::New(const FilePath &path, ui32 subMesh, f32 globalScale, bool isUseFileScale)
{
	struct Proxy final : public MeshPathAssetIdentification
	{
		Proxy(const FilePath &path, ui32 subMesh, f32 globalScale, bool isUseFileScale) : MeshPathAssetIdentification(path, subMesh, globalScale, isUseFileScale)
		{}
	};
	return make_shared<Proxy>(path, subMesh, globalScale, isUseFileScale);
}

bool MeshPathAssetIdentification::operator == (const AssetIdentification &other) const
{
	if (other.AssetTypeId() != MeshAsset::GetTypeId())
	{
		return false;
	}
	auto casted = static_cast<const MeshPathAssetIdentification &>(other);
	return std::tie(_path, _subMesh, _globalScale) == std::tie(casted._path, casted._subMesh, casted._globalScale);
}

uiw MeshPathAssetIdentification::operator () () const
{
	return std::hash<FilePath>()(_path) ^ _subMesh ^ std::hash<f32>()(_globalScale);
}

TypeId MeshPathAssetIdentification::AssetTypeId() const
{
	return MeshAsset::GetTypeId();
}

const FilePath &MeshPathAssetIdentification::AssetFilePath() const
{
	return _path;
}

ui32 MeshPathAssetIdentification::SubMeshIndex() const
{
	return _subMesh;
}

f32 MeshPathAssetIdentification::GlobalScale() const
{
	return _globalScale;
}

bool MeshPathAssetIdentification::IsUseFileScale() const
{
	return _isUseFileScale;
}

MeshPathAssetIdentification::MeshPathAssetIdentification(const FilePath &path, ui32 subMesh, f32 globalScale, bool isUseFileScale) : _path(path), _subMesh(subMesh), _globalScale(globalScale), _isUseFileScale(isUseFileScale)
{
}

shared_ptr<PhysicsPropertiesAssetIdentification> PhysicsPropertiesAssetIdentification::New(PhysicsProperties &&properties)
{
	struct Proxy final : public PhysicsPropertiesAssetIdentification
	{
		Proxy(PhysicsProperties &&properties) : PhysicsPropertiesAssetIdentification(move(properties))
		{}
	};
	return make_shared<Proxy>(move(properties));
}

bool PhysicsPropertiesAssetIdentification::operator == (const AssetIdentification &other) const
{
	if (other.AssetTypeId() != PhysicsPropertiesAsset::GetTypeId())
	{
		return false;
	}
	auto casted = static_cast<const PhysicsPropertiesAssetIdentification &>(other);
	return _properties == casted._properties;
}

uiw PhysicsPropertiesAssetIdentification::operator () () const
{
	return _properties.Hash();
}

TypeId PhysicsPropertiesAssetIdentification::AssetTypeId() const
{
	return PhysicsPropertiesAsset::GetTypeId();
}

const PhysicsProperties &PhysicsPropertiesAssetIdentification::Properties() const
{
	return _properties;
}

PhysicsPropertiesAssetIdentification::PhysicsPropertiesAssetIdentification(PhysicsProperties &&properties) : _properties(move(properties))
{
}
