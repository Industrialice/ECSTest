#pragma once

#include "Component.hpp"

namespace ECSTest
{
    struct LightComponent : public _BaseComponent<LightComponent>
    {
        virtual pair<const TypeId *, uiw> Excludes() const override;

        static constexpr bool IsExclusive() 
        { 
            return false; 
        }
    };

	using LightComponentsArray = ArrayOfComponents<LightComponent>;

    GENERATE_TYPE_ID_TO_TYPE(LightComponent);
}