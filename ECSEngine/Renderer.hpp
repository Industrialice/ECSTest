#pragma once

#include "Components.hpp"

namespace ECSEngine
{
    INDIRECT_SYSTEM(Renderer)
    {
        void Accept(const Array<Position> &, const Array<Rotation> &, const Array<Scale> *, const Array<Parent> *, const Array<MeshRenderer> *, const Array<SkinnedMeshRenderer> *, Array<Camera> *, RequiredComponentAny<MeshRenderer, SkinnedMeshRenderer, Camera>);
    };
}