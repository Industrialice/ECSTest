#include "PreHeader.hpp"
#include "SystemsManagerMT.hpp"
#include "SystemsManagerST.hpp"

using namespace ECSTest;

shared_ptr<SystemsManager> SystemsManager::New(bool isMultiThreaded, const shared_ptr<LoggerType> &logger)
{
    //if (isMultiThreaded)
    //{
    //    return SystemsManagerMT::New(logger);
    //}
    return SystemsManagerST::New(logger);
}