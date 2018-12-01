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
