#include "PreHeader.hpp"
#include "IKeyController.hpp"

using namespace ECSTest;

void IKeyController::RemoveListener(IKeyController *instance, void *handle)
{
    return instance->RemoveListener(*(ListenerHandle *)handle);
}
