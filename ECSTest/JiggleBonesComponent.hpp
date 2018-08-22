#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct JiggleBonesComponent final : public _BaseComponent<JiggleBonesComponent>
    {
        Vector3 initialRotations{};
        Vector3 currentPosition{};
    };
}