#pragma once

namespace ECSEngine
{
    COMPONENT(Position)
    {
        Vector3 position;
    };

    COMPONENT(Rotation)
    {
        Quaternion rotation;
    };

    COMPONENT(Scale)
    {
        Vector3 scale;
    };
}