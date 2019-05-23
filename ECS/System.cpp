#include "PreHeader.hpp"
#include "System.hpp"

using namespace ECSTest;

BaseIndirectSystem *System::AsIndirectSystem()
{
	return nullptr;
}

const BaseIndirectSystem *System::AsIndirectSystem() const
{
	return nullptr;
}

BaseDirectSystem *System::AsDirectSystem()
{
	return nullptr;
}

const BaseDirectSystem *System::AsDirectSystem() const
{
	return nullptr;
}

IKeyController *System::GetKeyController()
{
    return _keyController.get();
}

const IKeyController *System::GetKeyController() const
{
    return _keyController.get();
}

void System::SetKeyController(const shared_ptr<IKeyController> &controller)
{
    _keyController = controller;
}

BaseIndirectSystem *BaseIndirectSystem::AsIndirectSystem()
{
	return this;
}

const BaseIndirectSystem *BaseIndirectSystem::AsIndirectSystem() const
{
	return this;
}

BaseDirectSystem *BaseDirectSystem::AsDirectSystem()
{
	return this;
}

const BaseDirectSystem *BaseDirectSystem::AsDirectSystem() const
{
	return this;
}
