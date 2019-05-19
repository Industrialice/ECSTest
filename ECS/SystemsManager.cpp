#include "PreHeader.hpp"
#include "SystemsManagerST.hpp"

using namespace ECSTest;

shared_ptr<SystemsManager> SystemsManager::New(bool isMultiThreaded, const shared_ptr<LoggerType> &logger)
{
	ASSUME(isMultiThreaded == false);
    return SystemsManagerST::New(logger);
}