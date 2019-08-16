#include "PreHeader.hpp"
#include "AssetIdMapper.hpp"

using namespace ECSEngine;

AssetId AssetIdMapper::Register(const shared_ptr<AssetIdentification> &assetIdentification)
{
	if (!assetIdentification)
	{
		SOFTBREAK;
		return {};
	}

	auto pathInsertResult = _assetPathToId.insert({assetIdentification, {AssetId(_currentAssetId), assetIdentification->AssetTypeId()}});
	if (pathInsertResult.second) // registered a new path
	{
		auto idInsertResult = _assetIdToPath.insert({AssetId(_currentAssetId), assetIdentification});
		ASSUME(idInsertResult.second);
		++_currentAssetId;
		ASSUME(_currentAssetId > 0); // check for overflow
	}

	if (pathInsertResult.first->second.second != assetIdentification->AssetTypeId())
	{
		SOFTBREAK;
		return {};
	}

	return pathInsertResult.first->second.first;
}

auto AssetIdMapper::Resolve(AssetId id) const -> const AssetIdentification *
{
	auto found = _assetIdToPath.find(id);
	if (found == _assetIdToPath.end())
	{
		return nullptr;
	}
	return &*found->second;
}
