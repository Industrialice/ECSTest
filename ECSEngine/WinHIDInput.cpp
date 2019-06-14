#include "PreHeader.hpp"
#include "WinHIDInput.hpp"
#include <..\External\HID\hidusage.h>

using namespace ECSEngine;

namespace ECSEngine
{
    auto GetPlatformMapping() -> const array<KeyCode, 256> &; // from WinVirtualKeysMapping.cpp
}

HIDInput::~HIDInput()
{
	Unregister();
}

bool HIDInput::Register(HWND hwnd)
{
	if (_window != 0)
	{
		SOFTBREAK;
		Unregister();
	}

    UINT devicesCount = 0;
    if (GetRawInputDeviceList(NULL, &devicesCount, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
    {
        SENDLOG(Error, HIDInput, "Register failed, GetRawInputDeviceList failed\n");
        return false;
    }

    unique_ptr<RAWINPUTDEVICELIST[]> deviceList{new RAWINPUTDEVICELIST[devicesCount]};

    devicesCount = GetRawInputDeviceList(deviceList.get(), &devicesCount, sizeof(RAWINPUTDEVICELIST));
    if (devicesCount == (UINT)-1)
    {
        SENDLOG(Error, HIDInput, "Register failed, GetRawInputDeviceList failed\n");
        return false;
    }

    SENDLOG(Info, HIDInput, "Detected %u HID devices\n", devicesCount);

    for (UINT index = 0; index < devicesCount; ++index)
    {
        char name[256];
        UINT sizeofName = sizeof(name);
        if (GetRawInputDeviceInfoA(deviceList[index].hDevice, RIDI_DEVICENAME, name, &sizeofName) == (UINT)-1)
        {
            SENDLOG(Warning, HIDInput, "Failed to fetch HID device %u name\n", index);
            continue;
        }

        const char *deviceTypeName = "undefined";
        switch (deviceList[index].dwType)
        {
        case RIM_TYPEHID:
            deviceTypeName = "HID";
            break;
        case RIM_TYPEKEYBOARD:
            deviceTypeName = "Keyboard";
            break;
        case RIM_TYPEMOUSE:
            deviceTypeName = "Mouse";
            break;
        }

        //SENDLOG(Info, HIDInput, "HID device %u name is %s, type is %s\n", index, name, deviceTypeName);
    }
	
	array<RAWINPUTDEVICE, 2> hids;

	hids[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	hids[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	hids[0].dwFlags = 0; // RIDEV_NOLEGACY causes always busy state
	hids[0].hwndTarget = NULL;

	hids[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	hids[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	hids[1].dwFlags = RIDEV_NOLEGACY | RIDEV_APPKEYS;
	hids[1].hwndTarget = NULL;

	if (!RegisterRawInputDevices(hids.data(), static_cast<UINT>(hids.size()), sizeof(RAWINPUTDEVICE)))
	{
		SENDLOG(Error, HIDInput, "Register failed, RegisterRawInputDevices failed\n");
		return false;
	}

	SENDLOG(Info, HIDInput, "Register's completed\n");
	_window = hwnd;
	return true;
}

void HIDInput::Unregister()
{
	if (_window == 0)
	{
		return;
	}

    // it's exactly the same as registering, except the flags are RIDEV_REMOVE
	array<RAWINPUTDEVICE, 2> hids;

	hids[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	hids[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	hids[0].dwFlags = RIDEV_REMOVE;
	hids[0].hwndTarget = NULL; // must be NULL here

	hids[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	hids[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	hids[1].dwFlags = RIDEV_REMOVE;
	hids[1].hwndTarget = NULL; // must be NULL here

	if (!RegisterRawInputDevices(hids.data(), static_cast<UINT>(hids.size()), sizeof(RAWINPUTDEVICE)))
	{
		SENDLOG(Error, HIDInput, "Unregister failed, RegisterRawInputDevices failed\n");
		SOFTBREAK;
	}

	SENDLOG(Info, HIDInput, "Unregister's completed\n");
	_window = 0;
}

void HIDInput::Dispatch(ControlsQueue &controlsQueue, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	RAWINPUT data;
	UINT dataSize = sizeof(data);
	if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &data, &dataSize, sizeof(RAWINPUTHEADER)) == -1)
	{
		return;
	}
	ASSUME(dataSize == sizeof(data));

	if (data.header.dwType == RIM_TYPEMOUSE)
	{
		const RAWMOUSE &mouse = data.data.mouse;

        USHORT lowerBit = mouse.usFlags & 0x1;
		if (lowerBit == MOUSE_MOVE_ABSOLUTE) // when you're connected through remote desktop, flags may be MOUSE_MOVE_ABSOLUTE | MOUSE_VIRTUAL_DESKTOP
		{
			HARDBREAK;
		}
		if (mouse.usFlags & MOUSE_MOVE_NOCOALESCE)
		{
			HARDBREAK;
		}

		if (mouse.lLastX || mouse.lLastY)
		{
            controlsQueue.Enqueue(DeviceTypes::MouseKeyboard, ControlAction::MouseMove{{mouse.lLastX, mouse.lLastY}});
		}

		if (mouse.usButtonFlags)
		{
			auto checkKey = [&controlsQueue, &mouse](USHORT windowsUpKey, USHORT windowsDownKey, KeyCode key)
			{
				if (mouse.usButtonFlags & windowsUpKey)
				{
                    controlsQueue.Enqueue(DeviceTypes::MouseKeyboard, ControlAction::Key{key, ControlAction::Key::KeyState::Released});
				}
				else if (mouse.usButtonFlags & windowsDownKey)
				{
                    controlsQueue.Enqueue(DeviceTypes::MouseKeyboard, ControlAction::Key{key, ControlAction::Key::KeyState::Pressed});
				}
			};

			checkKey(RI_MOUSE_BUTTON_1_UP, RI_MOUSE_BUTTON_1_DOWN, KeyCode::MButton0);
			checkKey(RI_MOUSE_BUTTON_2_UP, RI_MOUSE_BUTTON_2_DOWN, KeyCode::MButton1);
			checkKey(RI_MOUSE_BUTTON_3_UP, RI_MOUSE_BUTTON_3_DOWN, KeyCode::MButton2);
			checkKey(RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_4_DOWN, KeyCode::MButton3);
			checkKey(RI_MOUSE_BUTTON_5_UP, RI_MOUSE_BUTTON_5_DOWN, KeyCode::MButton4);

			if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
			{
				//  TODO: some kind of normalization?
                controlsQueue.Enqueue(DeviceTypes::MouseKeyboard, ControlAction::MouseWheel{(SHORT)mouse.usButtonData / -WHEEL_DELTA});
			}
		}
	}
	else if (data.header.dwType == RIM_TYPEKEYBOARD)  //  TODO: fix left and right controls and alt keys
	{
		const RAWKEYBOARD &kb = data.data.keyboard;

		if (kb.VKey > 255)
		{
			SOFTBREAK;
			return;
		}

        bool isReleasing = (kb.Flags & RI_KEY_BREAK) != 0;
        KeyCode key = GetPlatformMapping()[kb.VKey];

        // we need additional processing to distinguish between actual left keys and right keys
        if (key == KeyCode::LShift || key == KeyCode::LControl || key == KeyCode::LAlt || key == KeyCode::LEnter || key == KeyCode::LDelete)
        {
            if (key == KeyCode::LShift)
            {
                if (kb.MakeCode != 0x2A)
                {
                    key = KeyCode::RShift;
                }
            }
            else
            {
                bool hasE0Flag = (kb.Flags & RI_KEY_E0) != 0;

                if (key == KeyCode::LControl)
                {
                    if (hasE0Flag)
                    {
                        key = KeyCode::RControl;
                    }
                }
                else if (key == KeyCode::LAlt)
                {
                    if (hasE0Flag)
                    {
                        key = KeyCode::RAlt;
                    }
                }
                else if (key == KeyCode::LEnter)
                {
                    if (hasE0Flag)
                    {
                        key = KeyCode::REnter;
                    }
                }
                else
                {
                    ASSUME(key == KeyCode::LDelete);

                    if (hasE0Flag == false) // swapped
                    {
                        key = KeyCode::RDelete;
                    }
                }
            }
        }

        controlsQueue.Enqueue(DeviceTypes::MouseKeyboard, ControlAction::Key{key, isReleasing ? ControlAction::Key::KeyState::Released : ControlAction::Key::KeyState::Pressed});
	}
}
