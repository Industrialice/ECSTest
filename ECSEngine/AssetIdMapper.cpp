#include "PreHeader.hpp"
#include "AssetIdMapper.hpp"

using namespace ECSEngine;

AssetId AssetIdMapper::Register(const FilePath &pathToAsset, TypeId assetType)
{
	auto pathInsertResult = _assetPathToId.insert({pathToAsset, {AssetId(_currentAssetId), assetType}});
	if (pathInsertResult.second) // registered a new path
	{
		auto idInsertResult = _assetIdToPath.insert({AssetId(_currentAssetId), {pathToAsset, assetType}});
		ASSUME(idInsertResult.second);
		++_currentAssetId;
		ASSUME(_currentAssetId > 0); // check for overflow
	}

	if (pathInsertResult.first->second.second != assetType)
	{
		SOFTBREAK;
		return {};
	}

	return pathInsertResult.first->second.first;
}

optional<pair<FilePath, TypeId>> AssetIdMapper::ResolveIdToPath(AssetId id) const
{
	auto found = _assetIdToPath.find(id);
	if (found == _assetIdToPath.end())
	{
		return nullopt;
	}
	return found->second;
}
