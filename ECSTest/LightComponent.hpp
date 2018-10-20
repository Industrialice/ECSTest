#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct LightComponent : public _BaseComponent<LightComponent>
    {
    };

    GENERATE_TYPE_ID_TO_TYPE(LightComponent);
}