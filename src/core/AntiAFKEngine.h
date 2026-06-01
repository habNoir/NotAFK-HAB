#pragma once
#include <windows.h>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include "../utils/ConfigManager.h"

namespace HAB::Core {
    class AntiAFKEngine {
    public:
        static AntiAFKEngine& GetInstance();
        
        // Gunakan HAB::Settings agar dikenali dari cakupan namespace mana pun
        void Start(HAB::Settings& settings, HWND hwndOwner);
        void Stop(HAB::Settings& settings);
        
        bool IsRunning() const;
        void ForceTriggerAction(const HAB::Settings& settings);
        
        uint64_t GetSessionElapsedSeconds() const;
        int GetSecondsRemaining() const;

    private:
        AntiAFKEngine() = default;
        ~AntiAFKEngine();

        void EngineLoop(HWND hwndOwner);
        void MonitorActivityLoop();
        void ExecuteAntiIdleAction(HWND target, int actionType);
        void ExecuteAutoReset(HWND target);
        bool CheckAndPerformReconnect(HWND target);
        void RestoreUserWindow(HWND prevWnd, int method);
        void RestoreForegroundWindow(HWND prevWnd);
        void RestorePreviousWindowWithAltTab();
        void SmoothMoveMouse(int startX, int startY, int endX, int endY, int steps, int durationMs);

        std::thread m_engineThread;
        std::thread m_monitorThread;
        std::atomic<bool> m_running{ false };
        std::atomic<bool> m_stopMonitor{ false };
        std::atomic<bool> m_userActive{ false };
        std::atomic<uint64_t> m_lastActivityTime{ 0 };
        std::atomic<int> m_reminderState{ 0 }; 
        
        std::atomic<uint64_t> m_sessionStartTime{ 0 };
        std::atomic<int> m_secondsRemaining{ 0 };

        std::condition_variable m_cv;
        std::mutex m_mutex;
        HAB::Settings* m_settings = nullptr;
    };
}