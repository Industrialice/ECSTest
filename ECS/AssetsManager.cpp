#include "PreHeader.hpp"
#include <UniqueIdManager.hpp>
#include "AssetsManager.hpp"

using namespace ECSTest;

void AssetsManager::AddAssetLoader(TypeId type, AssetLoaderFuncType &&func)
{
	auto [it, result] = _assetLoaders.insert({type, func});
	if (!result)
	{
		SOFTBREAK; // such loader already exists
	}
}

void AssetsManager::RemoveAssetLoader(TypeId type)
{
	auto removedCount = _assetLoaders.erase(type);
	if (removedCount != 1)
	{
		SOFTBREAK; // such loader does not exist
	}
}