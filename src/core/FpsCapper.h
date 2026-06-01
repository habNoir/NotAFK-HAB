#pragma once
#include <windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace HAB::Core {
    class FpsCapper {
    public:
        static FpsCapper& GetInstance();
        void Start(int limit, bool unlockOnFocus, bool fishstrapSupport);
        void Stop();
        void SetLimit(int limit);
        void SetUnlockOnFocus(bool unlock);
        void Pause(bool pause);
        bool IsPaused() const;

    private:
        FpsCapper() = default;
        ~FpsCapper();
        void WorkerLoop();

        std::thread m_workerThread;
        std::atomic<bool> m_running{ false };
        std::atomic<bool> m_paused{ false };
        std::atomic<int> m_fpsLimit{ 0 };
        std::atomic<bool> m_unlockOnFocus{ false };
        std::atomic<bool> m_fishstrap{ false };
        std::mutex m_mutex;
        std::vector<HANDLE> m_threadHandles;
    };
}