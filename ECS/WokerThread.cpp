#include "PreHeader.hpp"
#include "WokerThread.hpp"

using namespace ECSTest;

WorkerThread::~WorkerThread()
{
    if (_threadData)
    {
        if (!_threadData->isExiting)
        {
            // destroying a running worker is not allowed
            HARDBREAK;
        }
        if (_threadData->thread.joinable())
        {
            // destroying a thread that hasn't been joined is a performance hazard
            // may not be what you wanted
            SOFTBREAK;
            _threadData->thread.join();
        }
    }
}

WorkerThread::WorkerThread(WorkerThread &&source) : _threadData(source._threadData.release())
{}

WorkerThread &WorkerThread::operator = (WorkerThread &&source)
{
    ASSUME(this != &source);
    if (_threadData)
    {
        if (!_threadData->isExiting)
        {
            // destroying a running worker is not allowed
            HARDBREAK;
        }
        if (_threadData->thread.joinable())
        {
            // destroying a thread that hasn't been joined is a performance hazard
            // may not be what you wanted
            SOFTBREAK;
            _threadData->thread.join();
        }
    }
    _threadData.reset(source._threadData.release());
    return *this;
}

void WorkerThread::AddWork(std::function<void()> &&work)
{
    std::scoped_lock lock{_threadData->workAccessMutex};
    _threadData->work.push(move(work));
    _threadData->workCount.fetch_add(1);
    _threadData->newStateNotifier.notify_all();
}

void WorkerThread::Start()
{
    if (IsRunning())
    {
        SOFTBREAK;
        return;
    }
    _threadData->isExiting = false;
    _threadData->thread = std::thread(ThreadLoop, std::ref(*_threadData.get()));
}

bool WorkerThread::IsRunning() const
{
    return !_threadData->isExiting;
}

void WorkerThread::Stop()
{
    if (_threadData->isExiting)
    {
        return;
    }
    _threadData->isExiting = true;
    std::scoped_lock lock{_threadData->workAccessMutex};
    _threadData->newStateNotifier.notify_all();
}

void WorkerThread::Join()
{
    if (!_threadData->isExiting)
    {
        HARDBREAK;
    }
    _threadData->thread.join();
}

uiw WorkerThread::WorkInProgressCount() const
{
    return _threadData->workCount.load();
}

void WorkerThread::SetOnWorkDoneNotifier(const shared_ptr<pair<std::mutex, std::condition_variable>> &notifier)
{
    _threadData->onWorkDoneNotifier = notifier;
}

void WorkerThread::ThreadLoop(ThreadData &data)
{
    while (!data.isExiting)
    {
        std::unique_lock workWaitLock{data.workAccessMutex};
        data.newStateNotifier.wait(workWaitLock, [&data] { return data.workCount > 0 || data.isExiting; });
        workWaitLock.unlock();

        while (data.workCount.load())
        {
            std::function<void()> work;
            {
                std::scoped_lock lock{data.workAccessMutex};
                ASSUME(data.work.size() == data.workCount.load());
                work = data.work.top();
                data.work.pop();
            }

            work();

            data.workCount.fetch_sub(1);

            auto onWorkDoneNotifier = data.onWorkDoneNotifier; // copy it in case somebody rewrites while we're working with it
            if (onWorkDoneNotifier)
            {
                std::unique_lock doneLock{onWorkDoneNotifier->first};
                onWorkDoneNotifier->second.notify_all();
            }
        }
    }
}
