#pragma once

namespace ECSEngine::Scene
{
    unique_ptr<IEntitiesStream> Create(EntityIDGenerator &idGenerator);
}