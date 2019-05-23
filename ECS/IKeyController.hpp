#pragma once

#include "VirtualKeys.hpp"
#include <ListenerHandle.hpp>

namespace ECSTest
{
    struct DeviceTypes
    {
        static constexpr struct DeviceType : EnumCombinable<DeviceType, ui32, true>
        {} _None = DeviceType::Create(0),
            MouseKeyboard = DeviceType::Create(1 << 0),
            Touch0 = DeviceType::Create(1 << 1),
            Touch1 = DeviceType::Create(1 << 2),
            Touch2 = DeviceType::Create(1 << 3),
            Touch3 = DeviceType::Create(1 << 4),
            Touch4 = DeviceType::Create(1 << 5),
            Touch5 = DeviceType::Create(1 << 6),
            Touch6 = DeviceType::Create(1 << 7),
            Touch7 = DeviceType::Create(1 << 8),
            Touch8 = DeviceType::Create(1 << 9),
            Touch9 = DeviceType::Create(1 << 10),
            Joystick0 = DeviceType::Create(1 << 11),
            Joystick1 = DeviceType::Create(1 << 12),
            Joystick2 = DeviceType::Create(1 << 13),
            Joystick3 = DeviceType::Create(1 << 14),
            Joystick4 = DeviceType::Create(1 << 15),
            Joystick5 = DeviceType::Create(1 << 16),
            Joystick6 = DeviceType::Create(1 << 17),
            Joystick7 = DeviceType::Create(1 << 18),
            _AllTouches = Touch0.Combined(Touch1).Combined(Touch2).Combined(Touch3).Combined(Touch4).Combined(Touch5).Combined(Touch6).Combined(Touch7).Combined(Touch8).Combined(Touch9),
            _AllJoysticks = Joystick0.Combined(Joystick1).Combined(Joystick2).Combined(Joystick3).Combined(Joystick4).Combined(Joystick5).Combined(Joystick6).Combined(Joystick7),
            _AllDevices = MouseKeyboard.Combined(_AllTouches).Combined(_AllJoysticks);
    };

	[[nodiscard]] inline ui32 DeviceIndex(DeviceTypes::DeviceTypes::DeviceType device)
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

    class ControlAction
    {
    public:
        struct Key
        {
            KeyCode key{};
            enum class KeyState { Released, Pressed, Repeated } keyState{};
        };
        struct MouseSetPosition
        {
            i32Vector2 position{};
        };
        struct MouseMove
        {
            i32Vector2 delta{};
        };
        struct MouseWheel
        {
            i32 delta{};
        };
        struct TouchDown
        {
            i32Vector2 position{};
        };
        struct TouchMove
        {
            i32Vector2 delta{};
        };
        struct TouchUp
        {
            i32Vector2 lastPosition{};
        };
        struct TouchLongPress
        {
            i32Vector2 position{};
        };
        struct TouchDoubleTap
        {
            i32Vector2 position{};
        };
        struct TouchZoomStart
        {
            i32Vector2 focusPoint{};
        };
        struct TouchZoom
        {
            i32Vector2 focusPoint{};
            f32 delta{};
        };
        struct TouchZoomEnd
        {};
        struct Custom
        {
            TypeId type{};
            shared_ptr<void> data{};
        };

        ControlAction() {}
        ControlAction(Key actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(MouseSetPosition actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(MouseMove actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(MouseWheel actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchDown actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchMove actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchUp actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchLongPress actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchDoubleTap actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchZoomStart actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchZoom actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchZoomEnd actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(const Custom &actionData, const TimeMoment &occuredAt, DeviceTypes::DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}

        template <typename T> [[nodiscard]] const T *Get() const
        {
            if (auto *typed = std::get_if<T>(&action); typed)
            {
                return typed;
            }
            return nullptr;
        }

        template <typename T> [[nodiscard]] T *Get()
        {
            if (auto *typed = std::get_if<T>(&action); typed)
            {
                return typed;
            }
            return nullptr;
        }

    public:
        TimeMoment occuredAt{};
        DeviceTypes::DeviceTypes::DeviceType device = DeviceTypes::_None;

    private:
        variant<Key, MouseSetPosition, MouseMove, MouseWheel, TouchDown, TouchMove, TouchUp, TouchLongPress, TouchDoubleTap, TouchZoomStart, TouchZoom, TouchZoomEnd, Custom> action{};
    };

    class ControlsQueue
    {
        vector<ControlAction> _actions;

    public:

        void clear()
        {
            _actions.clear();
        }

		[[nodiscard]] uiw size() const
        {
            return _actions.size();
        }

        void push_back(ControlAction &&action)
        {
            _actions.push_back(move(action));
        }

        void push_back(const ControlAction &action)
        {
            _actions.push_back(action);
        }

        template <typename T> void Enqueue(DeviceTypes::DeviceType device, T action)
        {
            ASSUME(device != DeviceTypes::_None);
            _actions.push_back(ControlAction{action, TimeMoment::Now(), device});
        }

        void Enqueue(Array<const ControlAction> actions)
        {
            _actions.insert(_actions.end(), actions.begin(), actions.end());
        }

		[[nodiscard]] std::experimental::generator<ControlAction> Enumerate() const
        {
            for (const auto &action : _actions)
            {
                co_yield action;
            }
        }

		[[nodiscard]] Array<const ControlAction> Actions() const
        {
            return {_actions.data(), _actions.size()};
        }
    };

    class IKeyController
    {
        static void RemoveListener(IKeyController *instance, void *handle);

    public:
        using ListenerCallbackType = function<bool(const ControlAction &action)>; // return true if the action needs to be blocked from going to any subsequent listeners
        using ListenerHandle = TListenerHandle<IKeyController, RemoveListener, ui32>;

        struct KeyInfo
        {
            using KeyState = ControlAction::Key::KeyState;
            KeyState keyState{};
            ui32 timesKeyStateChanged{};
            TimeMoment occuredAt = TimeMoment::Now();

            bool IsPressed() const
            {
                return keyState != KeyState::Released;
            }
        };

        using AllKeyStates = array<KeyInfo, (size_t)KeyCode::_size>;

        virtual ~IKeyController() = default;
        virtual void Dispatch(const ControlAction &action) = 0;
        virtual void Dispatch(std::experimental::generator<ControlAction> enumerable) = 0;
        virtual void Update() = 0; // may be used for key repeating
        [[nodiscard]] virtual KeyInfo GetKeyInfo(KeyCode key, DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const = 0; // always default for Touch
        [[nodiscard]] virtual optional<i32Vector2> GetPositionInfo(DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const = 0; // always nullopt for Joystick
        [[nodiscard]] virtual const AllKeyStates &GetAllKeyStates(DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const = 0; // always default for Touch
        [[nodiscard]] virtual ListenerHandle OnControlAction(const ListenerCallbackType &callback, DeviceTypes::DeviceType deviceMask = DeviceTypes::_AllDevices) = 0;
        virtual void RemoveListener(ListenerHandle &handle) = 0;
    };

    class EmptyKeyController : public IKeyController
    {
    protected:
        EmptyKeyController() = default;
        EmptyKeyController(EmptyKeyController &&) = delete;
        EmptyKeyController &operator = (EmptyKeyController &&) = delete;

    public:
        static shared_ptr<EmptyKeyController> New()
        {
            struct Proxy : public EmptyKeyController
            {
                Proxy() : EmptyKeyController() {}
            };
            return make_shared<Proxy>();
        }

        virtual ~EmptyKeyController() override = default;
        virtual void Dispatch(const ControlAction &action) override {}
        virtual void Dispatch(std::experimental::generator<ControlAction> enumerable) override {}
        virtual void Update() override {}
        [[nodiscard]] virtual KeyInfo GetKeyInfo(KeyCode key, DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const override { return {}; }
        [[nodiscard]] virtual optional<i32Vector2> GetPositionInfo(DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const override { return {}; }
        [[nodiscard]] virtual const AllKeyStates &GetAllKeyStates(DeviceTypes::DeviceType device = DeviceTypes::MouseKeyboard) const override { return _defaultKeyStates; }
        [[nodiscard]] virtual ListenerHandle OnControlAction(const ListenerCallbackType &callback, DeviceTypes::DeviceType deviceMask = DeviceTypes::_AllDevices) override { return {}; }
        virtual void RemoveListener(ListenerHandle &handle) override {}

    private:
        const AllKeyStates _defaultKeyStates{};
    };
}