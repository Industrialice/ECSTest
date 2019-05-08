#pragma once

namespace ECSEngine::Scene
{
	[[nodiscard]] unique_ptr<IEntitiesStream> Create(EntityIDGenerator &idGenerator);
}