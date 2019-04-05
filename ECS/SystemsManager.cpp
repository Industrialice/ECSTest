#include "PreHeader.hpp"
#include "SystemsManagerMT.hpp"
#include "SystemsManagerST.hpp"

using namespace ECSTest;

shared_ptr<SystemsManager> SystemsManager::New(bool isMultiThreaded)
{
    if (isMultiThreaded)
    {
        return SystemsManagerMT::New();
    }
    return SystemsManagerST::New();
}