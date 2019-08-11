#pragma once

#include "AssetIdMapper.hpp"

namespace ECSEngine::Scene
{
	[[nodiscard]] unique_ptr<IEntitiesStream> Create(EntityIDGenerator &idGenerator, AssetIdMapper &assetIdMapper);
}