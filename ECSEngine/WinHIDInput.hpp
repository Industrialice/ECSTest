#pragma once

#include "IKeyController.hpp"

namespace ECSEngine
{
	class HIDInput
	{
		HWND _window = 0;

	public:
		~HIDInput();
		[[nodiscard]] bool Register(HWND hwnd);
		void Unregister();
		void Dispatch(ControlsQueue &controlsQueue, HWND hwnd, WPARAM wParam, LPARAM lParam);
	};
}