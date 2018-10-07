#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct SoundEmitterComponent : public _BaseComponent<SoundEmitterComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const override;

        static constexpr bool IsExclusive()
        {
            return false;
        }
    };

    GENERATE_TYPE_ID_TO_TYPE(SoundEmitterComponent);
}