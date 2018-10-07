#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct JiggleBonesComponent final : public _BaseComponent<JiggleBonesComponent>
    {
        Vector3 initialRotations{};
        Vector3 currentPosition{};

        virtual pair<const TypeId *, uiw> Excludes() const;
    };

    GENERATE_TYPE_ID_TO_TYPE(JiggleBonesComponent);
}