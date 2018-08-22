#pragma once

#include "Component.hpp"

namespace ECSTest
{
    class System
    {
    public:
        struct RequiredComponent
        {
            TypeId type;
            bool isReadAccess;
            bool isWriteAccess;
        };

        virtual pair<const RequiredComponent *, uiw> RequiredComponents() const = 0;
    };
}