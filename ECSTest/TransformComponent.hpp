#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct TransformComponent final : public _BaseComponent<TransformComponent>
    {
        enum class Mutability
        {
            Dynamic,
            Unmovable,
            Static
        };

        Vector3 position = {0, 0, 0};
        Vector3 rotation = {0, 0, 0};
        Vector3 scale = {1, 1, 1};
        Mutability mutability = Mutability::Dynamic;
    };

    GENERATE_TYPE_ID_TO_TYPE(TransformComponent);
}