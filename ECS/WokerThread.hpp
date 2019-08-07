#pragma once

namespace ECSTest
{
    class WorkerThread
    {
        struct ThreadData
        {
            std::thread thread{};
            std::atomic<bool> isExiting{true};
            std::mutex workAccessMutex{}; // if the thread is not joined, it must be acquired before you can access work
            std::stack<std::function<void()>> work{};
            std::condition_variable newStateNotifier{};
            std::atomic<ui32> workCount{};
            shared_ptr<pair<std::mutex, std::condition_variable>> onWorkDoneNotifier{};
        };
        unique_ptr<ThreadData> _threadData = make_unique<ThreadData>(); // the pointer will be referenced by the thread

    public:
        ~WorkerThread();
        WorkerThread() = default;
        WorkerThread(WorkerThread &&source) noexcept;
        WorkerThread &operator = (WorkerThread &&source) noexcept;
        void AddWork(std::function<void()> &&work);
        void Start();
		[[nodiscard]] bool IsRunning() const;
        void Stop();
        void Join();
		[[nodiscard]] uiw WorkInProgressCount() const;
        void SetOnWorkDoneNotifier(const shared_ptr<pair<std::mutex, std::condition_variable>> &notifier);

    private:
        static void ThreadLoop(ThreadData &data);
    };
}