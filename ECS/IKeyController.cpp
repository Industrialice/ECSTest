#include "PreHeader.hpp"
#include "IKeyController.hpp"

using namespace ECSTest;

// ControlsQueue

void ControlsQueue::clear()
{
	_actions.clear();
}

uiw ControlsQueue::size() const
{
	return _actions.size();
}

void ControlsQueue::push_back(ControlAction &&action)
{
	_actions.push_back(move(action));
}

void ControlsQueue::push_back(const ControlAction &action)
{
	_actions.push_back(action);
}

void ControlsQueue::Enqueue(Array<const ControlAction> actions)
{
	_actions.insert(_actions.end(), actions.begin(), actions.end());
}

auto ControlsQueue::begin() -> iteratorType
{
	return _actions.begin();
}

auto ControlsQueue::begin() const -> constIteratorType
{
	return _actions.cbegin();
}

auto ControlsQueue::end() -> iteratorType
{
	return _actions.end();
}

auto ControlsQueue::end() const -> constIteratorType
{
	return _actions.cend();
}

auto ControlsQueue::Actions() const -> Array<const ControlAction>
{
	return {_actions.data(), _actions.size()};
}

// Others

void IKeyController::RemoveListener(IKeyController *instance, void *handle)
{
    return instance->RemoveListener(*static_cast<ListenerHandle *>(handle));
}

bool IKeyController::KeyInfo::IsPressed() const
{
	return keyState != KeyState::Released;
}

uiw DeviceIndex(DeviceTypes::DeviceTypes::DeviceType device)
{
	ASSUME(Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) == Funcs::IndexOfLeastSignificantNonZeroBit(device.AsInteger()));
	if (device >= DeviceTypes::Touch0 && device <= DeviceTypes::Touch9)
	{
		return Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Touch0.AsInteger());
	}
	if (device >= DeviceTypes::Joystick0 && device <= DeviceTypes::Joystick7)
	{
		return Funcs::IndexOfMostSignificantNonZeroBit(device.AsInteger()) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceTypes::Joystick0.AsInteger());
	}
	return 0;
}

shared_ptr<EmptyKeyController> EmptyKeyController::New()
{
	struct Proxy : public EmptyKeyController
	{
		using EmptyKeyController::EmptyKeyController;
	};
	return make_shared<Proxy>();
}