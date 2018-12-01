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

        // these are cached fields that account for all parent transformations
        Vector3 _globalPosition = {0, 0, 0};
        Vector3 _globalRotation = {0, 0, 0};
        Vector3 _globalScale = {1, 1, 1};
        Mutability _globalMutability = Mutability::Dynamic; // the lowest Mutability between this transformation and its parents' transoformations

        Vector3 position = {0, 0, 0};
        Vector3 rotation = {0, 0, 0};
        Vector3 scale = {1, 1, 1};
        Mutability mutability = Mutability::Dynamic;
    };

    GENERATE_TYPE_ID_TO_TYPE(TransformComponent);
}