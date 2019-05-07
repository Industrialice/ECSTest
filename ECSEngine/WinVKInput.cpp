#include "PreHeader.hpp"
#include "WinVKInput.hpp"

using namespace ECSEngine;

namespace ECSEngine
{
    auto GetPlatformMapping() -> const array<KeyCode, 256> &; // from WinVirtualKeysMapping.cpp
}

void VKInput::Dispatch(ControlsQueue &controlsQueue, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_SETCURSOR || msg == WM_MOUSEMOVE)
	{
		POINTS mousePos = MAKEPOINTS(lParam);

		if (_mousePositionDefined == false)
		{
			_lastMouseX = mousePos.x;
			_lastMouseY = mousePos.y;
			_mousePositionDefined = true;
		}

		i32 deltaX = mousePos.x - _lastMouseX;
		i32 deltaY = mousePos.y - _lastMouseY;

		if (deltaX || deltaY)
		{
            controlsQueue.Enqueue(DeviceTypes::MouseKeyboard, ControlAction::MouseMove{{deltaX, deltaY}});

            _lastMouseX = mousePos.x;
            _lastMouseY = mousePos.y;
		}
	}
	else if (msg == WM_KEYDOWN || msg == WM_KEYUP)
	{
		if (wParam > 255)
		{
			SOFTBREAK;
			return;
		}

		KeyCode key = GetPlatformMapping()[wParam];

        controlsQueue.Enqueue(DeviceTypes::MouseKeyboard, ControlAction::Key{key, msg == WM_KEYDOWN ? ControlAction::Key::KeyState::Pressed : ControlAction::Key::KeyState::Released});
	}
	else
	{
		HARDBREAK;
	}
}
