#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct TransformComponent final : public _BaseComponent<TransformComponent>
    {
        enum Mutability
        {
            Dynamic,
            Static,
            Unmovable
        };

        Vector3 position = {0, 0, 0};
        Vector3 rotation = {0, 0, 0};
        Vector3 scale = {1, 1, 1};
        Mutability mutability = Mutability::Dynamic;
    };
}