#include "PreHeader.hpp"
#include "System.hpp"

using namespace ECSTest;

bool System::IsFatSystem() const
{
	return false;
}

void System::AcceptComponents(void *first, ...) const
{
    // do nothing in the base implementation
}
