#pragma once

#include "IKeyController.hpp"

// TODO: window went active/inactive, multiple windows handling

namespace ECSTest
{
	class KeyController : public std::enable_shared_from_this<KeyController>, public IKeyController
	{
    protected:
        KeyController() = default;
        KeyController(KeyController &&) = delete;
        KeyController &operator = (KeyController &&) = delete;

	public:
        static shared_ptr<KeyController> New();

        virtual ~KeyController() override = default;
		virtual void Dispatch(const ControlAction &action) override;
		virtual void Dispatch(std::experimental::generator<ControlAction> enumerable) override;
        virtual void Update() override;
        [[nodiscard]] virtual KeyInfo GetKeyInfo(KeyCode key, DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const override;
        [[nodiscard]] virtual optional<i32Vector2> GetPositionInfo(DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const override;
        [[nodiscard]] virtual const AllKeyStates &GetAllKeyStates(DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const override;
        [[nodiscard]] virtual ListenerHandle OnControlAction(const ListenerCallbackType &callback, DeviceTypes::DeviceType deviceMask = DeviceTypes::_AllDevices) override;
        virtual void RemoveListener(ListenerHandle &handle) override;

	private:
        struct MessageListener
        {
            ListenerCallbackType listener{};
            DeviceTypes::DeviceType deviceMask{};
            ui32 id{};
        };

        vector<MessageListener> _listeners{};
        bool _isListenersDirty = false;
        bool _isDispatchingInProgress = false;
        ui32 _currentId = 0;
        array<AllKeyStates, 1> _mouseKeyboardKeyStates{};
        array<AllKeyStates, 8> _joystickKeyStates{};
        const AllKeyStates _defaultKeyStates{};
        array<optional<i32Vector2>, 1> _mousePositionInfos{};
        array<optional<i32Vector2>, 10> _touchPositionInfos{};
	};
}