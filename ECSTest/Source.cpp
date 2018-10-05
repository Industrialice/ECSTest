#include "PreHeader.hpp"
#include "JiggleBonesComponent.hpp"
#include "BulletMoverComponent.hpp"
#include "SystemsManager.hpp"

using namespace ECSTest;

int main()
{
	StdLib::Initialization::PlatformAbstractionInitialize({});

	SystemsManager manager;
	manager.Spin({});

	printf("done\n");
	getchar();
    return 0;
}