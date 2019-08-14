#pragma once

#include "AssetIdMapper.hpp"

namespace ECSEngine::SceneFromMap
{
	[[nodiscard]] unique_ptr<IEntitiesStream> Create(const FilePath &pathToMap, const FilePath &pathToMapAssets, EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper);
}