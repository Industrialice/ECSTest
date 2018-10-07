#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct BulletMoverComponent final : public _BaseComponent<BulletMoverComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const;
    };

    GENERATE_TYPE_ID_TO_TYPE(BulletMoverComponent);
}