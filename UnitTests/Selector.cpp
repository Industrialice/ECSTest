#include <PreHeader.hpp>
#include "PreHeader.hpp"

shared_ptr<Logger<string_view, true>> Log;

void PerformUnitTests(bool isSuppressLogs);

void SimpleOrderTests();
void Benchmark();
void Benchmark2();
void KeyControllerTests();
void SyncTests();
void Falling();
void InteractionTests();
void ArgumentPassingTests();

namespace
{
	using FuncType = void(*)();
	constexpr FuncType AllTests[] =
	{
		SimpleOrderTests,
		KeyControllerTests,
		InteractionTests,
		ArgumentPassingTests,
		SyncTests,
		Benchmark,
		Falling,
		Benchmark2
	};
}

static void DoTest(void func())
{
	static ui32 index = 1;
	func();
	Log->Info("", "\nTest %u done\n\n", index);
	++index;
}

static void DoAllTests()
{
	for (auto func : AllTests)
	{
		DoTest(func);
	}
}

#ifdef PLATFORM_WINDOWS
int main()
{
#ifdef DEBUG
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_EVERY_16_DF);
#endif

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
			Position((screenWidth - static_cast<i32>(width)) / 2, (screenHeight - static_cast<i32>(height)) / 2).
			SnapToWindowSize(true);
	}

	PerformUnitTests(true);

	i32 value = 0;
	Log->Info("", "%i. All\n", value++);
	Log->Info("", "%i. SimpleOrderTests\n", value++);
	Log->Info("", "%i. KeyControllerTests\n", value++);
	Log->Info("", "%i. InteractionTests\n", value++);
	Log->Info("", "%i. ArgumentPassingTests\n", value++);
	Log->Info("", "%i. SyncTests\n", value++);
	Log->Info("", "%i. Benchmark\n", value++);
	Log->Info("", "%i. Falling\n", value++);
	Log->Info("", "%i. Benchmark2\n", value++);

restart:
    int choice = 0;
    scanf_s("%i", &choice);
	Log->Info("", "\n");

	if (choice == 0)
	{
		DoAllTests();
	}
	else if (static_cast<uiw>(choice) < CountOf(AllTests))
	{
		AllTests[choice - 1]();
	}
	else
	{
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