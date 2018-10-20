#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct SoundEmitterComponent : public _BaseComponent<SoundEmitterComponent>
    {
    };

    GENERATE_TYPE_ID_TO_TYPE(SoundEmitterComponent);
}