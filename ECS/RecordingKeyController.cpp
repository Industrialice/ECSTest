#include "PreHeader.hpp"
#include "RecordingKeyController.hpp"

using namespace ECSTest;

RecordingKeyController::RecordingKeyController(const shared_ptr<IKeyController> &nextController) : _nextController(nextController)
{
    ASSUME(nextController);
}

shared_ptr<RecordingKeyController> RecordingKeyController::New(const shared_ptr<IKeyController> &nextController)
{
    struct Proxy : public RecordingKeyController
    {
        Proxy(const shared_ptr<IKeyController> &nextController) : RecordingKeyController(nextController) {}
    };
    return make_shared<Proxy>(nextController);
}

void RecordingKeyController::Dispatch(const ControlAction &action)
{
    for (i32 index = static_cast<i32>(_recordingListenerHandles.size()) - 1; index >= 0; --index)
    {
        _recordingListenerHandles[index].listener(action);
    }
    return _nextController->Dispatch(action);
}

void RecordingKeyController::Dispatch(const ControlsQueue &controlsQueue)
{
	for (auto &action : controlsQueue)
	{
		Dispatch(action);
	}
}

//void RecordingKeyController::Dispatch(std::experimental::generator<ControlAction> enumerable)
//{
//    for (const auto &action : enumerable)
//    {
//        for (i32 index = (i32)_recordingListenerHandles.size() - 1; index >= 0; --index)
//        {
//            _recordingListenerHandles[index].listener(action);
//        }
//
//        _nextController->Dispatch(action);
//    }
//}

void RecordingKeyController::Update()
{
    return _nextController->Update();
}

auto RecordingKeyController::GetKeyInfo(KeyCode key, DeviceTypes::DeviceType device) const -> KeyInfo
{
    return _nextController->GetKeyInfo(key, device);
}

auto RecordingKeyController::GetPositionInfo(DeviceTypes::DeviceType device) const -> optional<i32Vector2>
{
    return _nextController->GetPositionInfo(device);
}

auto RecordingKeyController::GetAllKeyStates(DeviceTypes::DeviceType device) const -> const AllKeyStates &
{
    return _nextController->GetAllKeyStates(device);
}

auto RecordingKeyController::OnControlAction(const ListenerCallbackType &callback, DeviceTypes::DeviceType deviceMask) -> ListenerHandle
{
    return _nextController->OnControlAction(callback, deviceMask);
}

void RecordingKeyController::RemoveListener(ListenerHandle &handle)
{
    if (handle.Owner().expired())
    {
        return;
    }

    if (Funcs::AreSharedPointersEqual(handle.Owner(), shared_from_this()))
    {
        uiw index = 0;
        for (; ; ++index)
        {
            if (_recordingListenerHandles[index].id == handle.Id())
            {
                break;
            }
        }

        _recordingListenerHandles.erase(_recordingListenerHandles.begin() + index);

        handle.Owner().reset();
    }
    else
    {
        return _nextController->RemoveListener(handle);
    }
}

auto RecordingKeyController::OnRecordingControlAction(const ListenerCallbackType &callback) -> ListenerHandle
{
    ui32 id = AssignId<MessageListener, ui32, &MessageListener::id>(_currentId, _recordingListenerHandles.begin(), _recordingListenerHandles.end());
    _recordingListenerHandles.push_back({callback, id});
    return {shared_from_this(), id};
}
