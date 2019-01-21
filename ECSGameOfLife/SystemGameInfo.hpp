#pragma once

#include <System.hpp>

namespace ECSTest
{
    INDIRECT_SYSTEM(SystemGameInfo)
    {
		INDIRECT_ACCEPT_COMPONENTS();
    };
}