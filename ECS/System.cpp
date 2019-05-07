#include "PreHeader.hpp"
#include "System.hpp"

using namespace ECSTest;

IndirectSystem *System::AsIndirectSystem()
{
	return nullptr;
}

const IndirectSystem *System::AsIndirectSystem() const
{
	return nullptr;
}

DirectSystem *System::AsDirectSystem()
{
	return nullptr;
}

const DirectSystem *System::AsDirectSystem() const
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

IndirectSystem *IndirectSystem::AsIndirectSystem()
{
	return this;
}

const IndirectSystem *IndirectSystem::AsIndirectSystem() const
{
	return this;
}

DirectSystem *DirectSystem::AsDirectSystem()
{
	return this;
}

const DirectSystem *DirectSystem::AsDirectSystem() const
{
	return this;
}
