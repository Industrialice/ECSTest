#include <PreHeader.hpp>
#include <stdio.h>

void PerformUnitTests(bool isSuppressLogs);

void Benchmark();
void Benchmark2();
void KeyControllerTests();
void SyncTests();
void Falling();
void InteractionTests();
void ArgumentPassingTests();

static void DoTest(void func())
{
	static ui32 index = 1;
	func();
	printf("\nTest %u done\n\n", index);
	++index;
}

int main()
{
	StdLib::Initialization::Initialize({});

	#ifdef PLATFORM_WINDOWS
		auto console = NativeConsole(true);
		if (console)
		{
			ui32 screenWidth = GetSystemMetrics(SM_CXSCREEN);
			ui32 screenHeight = GetSystemMetrics(SM_CYSCREEN);
			ui32 preferredWidth = screenWidth / 4;
			ui32 preferredHeight = screenHeight / 4;
			ui32 width = std::max(preferredWidth, 750u);
			ui32 height = std::max(preferredHeight, 450u);
			console.
				BufferSize(std::nullopt, 150).
				Size(width, height).
				Position((screenWidth - (i32)width) / 2, (screenHeight - (i32)height) / 2).
				SnapToWindowSize(true);
		}
	#endif

	PerformUnitTests(true);

	printf("0. All\n");
    printf("1. KeyControllerTests\n");
    printf("2. InteractionTests\n");
	printf("3. ArgumentPassingTests\n");
    printf("4. SyncTests\n");
    printf("5. Benchmark\n");
    printf("6. Falling\n");
    printf("7. Benchmark2\n");

restart:
    int choice = 0;
    scanf_s("%i", &choice);
    printf("\n");

    switch (choice)
    {
	case 0:
		DoTest(KeyControllerTests);
		DoTest(InteractionTests);
		DoTest(ArgumentPassingTests);
		DoTest(SyncTests);
		DoTest(Benchmark);
		DoTest(Falling);
		DoTest(Benchmark2);
		break;
	case 1:
        KeyControllerTests();
        break;
    case 2:
        InteractionTests();
        break;
	case 3:
		ArgumentPassingTests();
		break;
    case 4:
        SyncTests();
        break;
    case 5:
        Benchmark();
        break;
    case 6:
        Falling();
        break;
    case 7:
        Benchmark2();
        break;
    default:
        printf("Incorrect input, try again\n");
        goto restart;
    }

    printf("Test complete\n\n");
    goto restart;
}