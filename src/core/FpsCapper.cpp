#include "FpsCapper.h"
#include "RobloxDetector.h"
#include <tlhelp32.h>

namespace HAB::Core {
    FpsCapper& FpsCapper::GetInstance() {
        static FpsCapper instance;
        return instance;
    }

    FpsCapper::~FpsCapper() {
        Stop();
    }

    void FpsCapper::Start(int limit, bool unlockOnFocus, bool fishstrapSupport) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_running) return;

        m_fpsLimit = limit;
        m_unlockOnFocus = unlockOnFocus;
        m_fishstrap = fishstrapSupport;
        m_running = true;
        m_paused = false;

        m_workerThread = std::thread(&FpsCapper::WorkerLoop, this);
    }

    void FpsCapper::Stop() {
        m_running = false;
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        for (HANDLE h : m_threadHandles) {
            ResumeThread(h);
            CloseHandle(h);
        }
        m_threadHandles.clear();
    }

    void FpsCapper::SetLimit(int limit) {
        m_fpsLimit = limit;
    }

    void FpsCapper::SetUnlockOnFocus(bool unlock) {
        m_unlockOnFocus = unlock;
    }

    void FpsCapper::Pause(bool pause) {
        m_paused = pause;
    }

    bool FpsCapper::IsPaused() const {
        return m_paused;
    }

    void FpsCapper::WorkerLoop() {
        auto lastUpdate = std::chrono::steady_clock::now();

        while (m_running) {
            int currentLimit = m_fpsLimit.load();
            if (currentLimit <= 0 || m_paused.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }

            long long frameDurationMs = 1000LL / currentLimit;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count();

            if (elapsed >= 2) {
                lastUpdate = now;

                std::vector<HANDLE> newHandles;
                HWND fgWnd = m_unlockOnFocus.load() ? GetForegroundWindow() : nullptr;
                std::vector<DWORD> pids = RobloxDetector::FindAllRobloxProcessIds(m_fishstrap.load());

                for (DWORD pid : pids) {
                    bool shouldCap = true;
                    if (fgWnd) {
                        DWORD fgPid = 0;
                        GetWindowThreadProcessId(fgWnd, &fgPid);
                        if (fgPid == pid) shouldCap = false;
                    }

                    if (shouldCap) {
                        HANDLE thSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
                        if (thSnap != INVALID_HANDLE_VALUE) {
                            THREADENTRY32 te = { 0 };
                            te.dwSize = sizeof(te);
                            if (Thread32First(thSnap, &te)) {
                                do {
                                    if (te.th32OwnerProcessID == pid) {
                                        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                                        if (hThread) newHandles.push_back(hThread);
                                    }
                                } while (Thread32Next(thSnap, &te));
                            }
                            CloseHandle(thSnap);
                        }
                    }
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                for (HANDLE h : m_threadHandles) {
                    ResumeThread(h);
                    CloseHandle(h);
                }
                m_threadHandles = std::move(newHandles);
            }

            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_threadHandles.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            } else {
                for (HANDLE h : m_threadHandles) SuspendThread(h);
                long long sleepTime = frameDurationMs - 1;
                if (sleepTime > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
                }
                for (HANDLE h : m_threadHandles) ResumeThread(h);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    }
}