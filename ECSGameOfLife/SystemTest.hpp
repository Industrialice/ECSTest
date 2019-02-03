#pragma once

#include <System.hpp>

#include "ComponentArtist.hpp"

namespace ECSTest
{
    INDIRECT_SYSTEM(SystemTest)
    {
        INDIRECT_ACCEPT_COMPONENTS(const Array<ComponentArtist> &artist);
    };
}