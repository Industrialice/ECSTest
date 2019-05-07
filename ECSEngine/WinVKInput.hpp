#pragma once

#include "IKeyController.hpp"

namespace ECSEngine
{
	class VKInput
	{
        i32 _lastMouseX{}, _lastMouseY{};
		bool _mousePositionDefined = false;

	public:
		void Dispatch(ControlsQueue &controlsQueue, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
}