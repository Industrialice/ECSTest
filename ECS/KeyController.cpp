#include "PreHeader.hpp"
#include "KeyController.hpp"
#include "Logger.hpp"

using namespace ECSTest;

shared_ptr<KeyController> KeyController::New()
{
    struct Proxy : public KeyController
    {
        Proxy() : KeyController() {}
    };
    return make_shared<Proxy>();
}

void KeyController::Dispatch(const ControlAction &action)
{
    if (_isDispatchingInProgress)
    {
        SOFTBREAK;
        return;
    }

    auto cookedAction = action;
    DeviceTypes::DeviceType value = cookedAction.device;

    if (auto mouseMoveAction = cookedAction.Get<ControlAction::MouseMove>(); mouseMoveAction)
    {
        if (_mousePositionInfos[0])
        {
            *_mousePositionInfos[0] += mouseMoveAction->delta;
        }
        else
        {
            _mousePositionInfos[0] = mouseMoveAction->delta;
        }
    }
    else if (auto keyAction = cookedAction.Get<ControlAction::Key>(); keyAction)
    {
        auto getKeyStates = [this, value]() -> AllKeyStates &
        {
            if (value == DeviceTypes::MouseKeyboard)
            {
                return _mouseKeyboardKeyStates[0];
            }
            uiw index = Funcs::IndexOfMostSignificantNonZeroBit(value.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Joystick0.AsInteger());
            return _joystickKeyStates[index];
        };

        auto &keyInfo = getKeyStates()[static_cast<size_t>(keyAction->key)];

        bool wasPressed = keyInfo.keyState == KeyInfo::KeyState::Pressed;
        bool isRepeating = wasPressed && keyAction->keyState == KeyInfo::KeyState::Pressed;

        ui32 currentKSCTimes = keyInfo.timesKeyStateChanged;
        if (keyInfo.keyState != keyAction->keyState || isRepeating)
        {
            ++currentKSCTimes;
        }

        keyInfo = {keyAction->keyState, currentKSCTimes, cookedAction.occuredAt};

        if (isRepeating)
        {
            keyAction->keyState = KeyInfo::KeyState::Repeated;
        }
    }
    else if (auto mouseSetPositionAction = cookedAction.Get<ControlAction::MouseSetPosition>(); mouseSetPositionAction)
    {
        _mousePositionInfos[0] = mouseSetPositionAction->position;
    }
    else if (auto touchDownAction = cookedAction.Get<ControlAction::TouchDown>(); touchDownAction)
    {
        uiw deviceIndex = Funcs::IndexOfMostSignificantNonZeroBit(value.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Touch0.AsInteger());
        _touchPositionInfos[deviceIndex] = touchDownAction->position;
    }
    else if (auto touchMoveAction = cookedAction.Get<ControlAction::TouchMove>(); touchMoveAction)
    {
        uiw deviceIndex = Funcs::IndexOfMostSignificantNonZeroBit(value.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Touch0.AsInteger());
        if (!_touchPositionInfos[deviceIndex])
        {
            SOFTBREAK;
            return;
        }
        *_touchPositionInfos[deviceIndex] += touchMoveAction->delta;
    }
    else if (auto touchUpAction = cookedAction.Get<ControlAction::TouchUp>(); touchUpAction)
    {
        uiw deviceIndex = Funcs::IndexOfMostSignificantNonZeroBit(value.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Touch0.AsInteger());
        if (!_touchPositionInfos[deviceIndex])
        {
            SOFTBREAK;
            return;
        }
        _touchPositionInfos[deviceIndex] = {};
    }

    _isDispatchingInProgress = true;
    for (i32 index = static_cast<i32>(_listeners.size()) - 1; index >= 0; --index)
    {
        const auto &listener = _listeners[index];
        if (listener.deviceMask.Contains(action.device))
        {
            if (listener.listener(cookedAction))
            {
                break;
            }
        }
    }
    _isDispatchingInProgress = false;

    if (_isListenersDirty)
    {
        _listeners.erase(std::remove_if(_listeners.begin(), _listeners.end(), [](const MessageListener &listener) { return listener.deviceMask == DeviceTypes::_None; }));
        _isListenersDirty = false;
    }
}

void KeyController::Dispatch(const ControlsQueue &controlsQueue)
{
	for (auto &action : controlsQueue)
	{
		Dispatch(action);
	}
}

//void KeyController::Dispatch(std::experimental::generator<ControlAction> enumerable)
//{
//    for (const auto &action : enumerable)
//    {
//        Dispatch(action);
//    }
//}

void KeyController::Update()
{}

auto KeyController::OnControlAction(const ListenerCallbackType &callback, DeviceTypes::DeviceType deviceMask) -> ListenerHandle
{
    if (deviceMask == DeviceTypes::_None) // not an error, but probably not what you wanted either
    {
        SOFTBREAK;
        return {};
    }

    ui32 id = AssignId<MessageListener, ui32, &MessageListener::id>(_currentId, _listeners.begin(), _listeners.end());
    _listeners.push_back({callback, deviceMask, id});
    return {shared_from_this(), id};
}

void KeyController::RemoveListener(ListenerHandle &handle)
{
    std::weak_ptr<ListenerHandle::ownerType> currentOwner{};
    handle.Owner().swap(currentOwner);
    if (currentOwner.expired())
    {
        return;
    }

    ASSUME(Funcs::AreSharedPointersEqual(currentOwner, shared_from_this()));

    uiw index = 0;
    for (; ; ++index)
    {
        if (_listeners[index].id == handle.Id())
        {
            break;
        }
    }

    if (_isDispatchingInProgress)
    {
        _listeners[index].deviceMask = DeviceTypes::_None;
        _isListenersDirty = true;
    }
    else
    {
        _listeners.erase(_listeners.begin() + index);
    }
}

auto KeyController::GetKeyInfo(KeyCode key, DeviceTypes::DeviceType device) const -> KeyInfo
{
    ASSUME(Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) == Funcs::IndexOfLeastSignificantNonZeroBit(device.AsInteger()));
    if (device == DeviceTypes::MouseKeyboard)
    {
        return _mouseKeyboardKeyStates[0][static_cast<ui32>(key)];
    }
    if (device >= DeviceTypes::Joystick0 && device <= DeviceTypes::Joystick7)
    {
        uiw index = Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Joystick0.AsInteger());
        return _joystickKeyStates[index][static_cast<ui32>(key)];
    }
    return {};
}

auto KeyController::GetPositionInfo(DeviceTypes::DeviceType device) const -> optional<i32Vector2>
{
    ASSUME(Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) == Funcs::IndexOfLeastSignificantNonZeroBit(device.AsInteger()));
    if (device == DeviceTypes::MouseKeyboard)
    {
        return _mousePositionInfos[0];
    }
    if (device >= DeviceTypes::Touch0 && device <= DeviceTypes::Touch9)
    {
        uiw index = Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Touch0.AsInteger());
        return _touchPositionInfos[index];
    }
    return {};
}

auto KeyController::GetAllKeyStates(DeviceTypes::DeviceType device) const -> const AllKeyStates &
{
    ASSUME(Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) == Funcs::IndexOfLeastSignificantNonZeroBit(device.AsInteger()));
    if (device == DeviceTypes::MouseKeyboard)
    {
        return _mouseKeyboardKeyStates[0];
    }
    if (device >= DeviceTypes::Joystick0 && device <= DeviceTypes::Joystick7)
    {
        uiw index = Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Joystick0.AsInteger());
        return _joystickKeyStates[index];
    }
    return _defaultKeyStates;
}
