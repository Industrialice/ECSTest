#include <PreHeader.hpp>
#include "PreHeader.hpp"

shared_ptr<Logger<string_view, true>> Log;

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
	Log->Info("", "\nTest %u done\n\n", index);
	++index;
}

static void DoAllTests()
{
	DoTest(KeyControllerTests);
	DoTest(InteractionTests);
	DoTest(ArgumentPassingTests);
	DoTest(SyncTests);
	DoTest(Benchmark);
	DoTest(Falling);
	DoTest(Benchmark2);
}

#ifdef PLATFORM_WINDOWS
int main()
{
	StdLib::Initialization::Initialize({});

	Log = make_shared<Logger<string_view, true>>();
	auto handle = Log->OnMessage(ECSTest::LogRecipient);

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

	PerformUnitTests(true);

	Log->Info("", "0. All\n");
	Log->Info("", "1. KeyControllerTests\n");
	Log->Info("", "2. InteractionTests\n");
	Log->Info("", "3. ArgumentPassingTests\n");
	Log->Info("", "4. SyncTests\n");
	Log->Info("", "5. Benchmark\n");
	Log->Info("", "6. Falling\n");
	Log->Info("", "7. Benchmark2\n");

restart:
    int choice = 0;
    scanf_s("%i", &choice);
	Log->Info("", "\n");

    switch (choice)
    {
	case 0:
		DoAllTests();
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
		Log->Error("", "Incorrect input, try again\n");
        goto restart;
    }

	Log->Info("", "Test complete\n\n");
    goto restart;
}
#else
int main(int argc, char **argv)
{
	StdLib::Initialization::Initialize({});
	Log = make_shared<Logger<string_view, true>>();
	auto handle = Log->OnMessage(ECSTest::LogRecipient);
	DoAllTests();
}
#endif