#include "AntiAFKEngine.h"
#include "InputSimulator.h"
#include "RobloxDetector.h"
#include "AudioManager.h"
#include "FpsCapper.h"
#include "../ui/StatusBarOverlay.h"
#include "../network/DiscordWebhook.h"
#include <chrono>

namespace HAB::Core {
    AntiAFKEngine& AntiAFKEngine::GetInstance() {
        static AntiAFKEngine instance;
        return instance;
    }

    AntiAFKEngine::~AntiAFKEngine() {
        if (m_running) {
            m_running = false;
            m_stopMonitor = true;
            m_cv.notify_all();
            if (m_engineThread.joinable()) m_engineThread.join();
            if (m_monitorThread.joinable()) m_monitorThread.join();
        }
    }

    bool AntiAFKEngine::IsRunning() const {
        return m_running.load();
    }

    uint64_t AntiAFKEngine::GetSessionElapsedSeconds() const {
        if (!m_running.load()) return 0;
        return (GetTickCount64() - m_sessionStartTime.load()) / 1000;
    }

    int AntiAFKEngine::GetSecondsRemaining() const {
        if (!m_running.load()) return 0;
        return m_secondsRemaining.load();
    }

    void AntiAFKEngine::Start(HAB::Settings& s, HWND hwndOwner) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_running) return;

        m_settings = &s;
        m_running = true;
        m_stopMonitor = false;
        m_reminderState = 0;
        m_userActive = false;
        m_lastActivityTime = GetTickCount64();
        
        m_sessionStartTime = GetTickCount64();
        m_secondsRemaining = s.selectedTime;

        m_engineThread = std::thread(&AntiAFKEngine::EngineLoop, this, hwndOwner);
        if (s.userSafeMode > 0 || s.afkReminder) {
            m_monitorThread = std::thread(&AntiAFKEngine::MonitorActivityLoop, this);
        }
    }

    void AntiAFKEngine::Stop(HAB::Settings& s) {
        if (!m_running) return;

        m_running = false;
        m_stopMonitor = true;
        m_cv.notify_all();

        if (m_engineThread.joinable()) m_engineThread.join();
        if (m_monitorThread.joinable()) m_monitorThread.join();

        uint64_t elapsed = (GetTickCount64() - m_sessionStartTime.load()) / 1000;
        s.totalAfkTime += elapsed;
        if (elapsed > s.longestAfkSession) {
            s.longestAfkSession = elapsed;
        }
        s.afkSessionsCompleted++;
        ConfigManager::Save(s);

        AudioManager::MuteAllRoblox(false, s.fishstrapSupport);
        RobloxDetector::RefreshWindowOpacity(true, false, false, false, s.fishstrapSupport);
    }

    void AntiAFKEngine::MonitorActivityLoop() {
        while (!m_stopMonitor) {
            LASTINPUTINFO lii = { 0 };
            lii.cbSize = sizeof(LASTINPUTINFO);
            if (GetLastInputInfo(&lii)) {
                if (lii.dwTime != static_cast<DWORD>(m_lastActivityTime.load())) {
                    m_lastActivityTime = lii.dwTime;
                    m_userActive = true;
                    m_reminderState = 0;
                } else if ((GetTickCount() - lii.dwTime) / 1000 >= 3) {
                    m_userActive = false;
                }
            }

            if (m_settings && m_settings->userSafeMode == 1) { 
                for (int i = 1; i < 256; i++) {
                    if (GetAsyncKeyState(i) & 0x8000) {
                        m_lastActivityTime = GetTickCount64();
                        m_userActive = true;
                        m_reminderState = 0;
                        break;
                    }
                }
            }

            if (m_settings && m_settings->afkReminder && !m_running && !m_userActive) {
                uint64_t inactiveTime = (GetTickCount64() - m_lastActivityTime.load()) / 1000;
                if (inactiveTime >= 18 * 60 && m_reminderState == 0) {
                    m_reminderState = 1;
                }
                if (inactiveTime >= 19 * 60 && m_reminderState == 1) {
                    m_reminderState = 2;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

    void AntiAFKEngine::ExecuteAntiIdleAction(HWND target, int actionType) {
        switch (actionType) {
            case 0: 
                InputSimulator::SendKeyPress(VK_SPACE, 20);
                break;
            case 1: 
                InputSimulator::SendKeyPress('W', 20);
                Sleep(20);
                InputSimulator::SendKeyPress('S', 20);
                break;
            case 2: 
                InputSimulator::SendKeyPress('I', 20);
                Sleep(20);
                InputSimulator::SendKeyPress('O', 20);
                break;
        }
    }

    void AntiAFKEngine::ExecuteAutoReset(HWND target) {
        InputSimulator::SendKeyPress(VK_ESCAPE, 20);
        Sleep(100);
        InputSimulator::SendKeyPress('R', 20);
        Sleep(100);
        InputSimulator::SendKeyPress(VK_RETURN, 20);
    }

    bool AntiAFKEngine::CheckAndPerformReconnect(HWND target) {
        if (!IsWindow(target) || !IsWindowVisible(target)) return false;

        RECT rc;
        if (!GetClientRect(target, &rc)) return false;
        int winW = rc.right - rc.left;
        int winH = rc.bottom - rc.top;

        int boxW = 400, boxH = 250;
        int checkX = (winW - boxW) / 2 + 10;
        int checkY = (winH - boxH) / 2 + 10;

        POINT pt = { checkX, checkY };
        ClientToScreen(target, &pt);

        HDC hdc = GetDC(nullptr);
        if (!hdc) return false;
        COLORREF color = GetPixel(hdc, pt.x, pt.y);
        ReleaseDC(nullptr, hdc);

        if (color != RGB(57, 59, 61)) return false;

        int btnW = 161, btnH = 34;
        int clickX = (winW - boxW) / 2 + boxW - btnW - 27 + (rand() % btnW);
        int clickY = (winH - boxH) / 2 + boxH - btnH - 21 + (rand() % btnH);

        POINT clickPt = { clickX, clickY };
        ClientToScreen(target, &clickPt);

        POINT oldCursor;
        GetCursorPos(&oldCursor);

        SmoothMoveMouse(oldCursor.x, oldCursor.y, clickPt.x, clickPt.y, 20, 80);
        Sleep(50);
        InputSimulator::PerformClick();
        Sleep(50);
        SetCursorPos(oldCursor.x, oldCursor.y);

        return true;
    }

    void AntiAFKEngine::SmoothMoveMouse(int startX, int startY, int endX, int endY, int steps, int durationMs) {
        double pi = 3.141592653589793;
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        for (int i = 1; i <= steps; ++i) {
            double prog = static_cast<double>(i) / steps;
            double factor = 0.5 * (1.0 - cos(prog * pi));

            int curX = static_cast<int>(startX + (endX - startX) * factor);
            int curY = static_cast<int>(startY + (endY - startY) * factor);

            long absX = curX * 65535 / screenW;
            long absY = curY * 65535 / screenH;

            INPUT input = { 0 };
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            input.mi.dx = absX;
            input.mi.dy = absY;
            SendInput(1, &input, sizeof(INPUT));

            if (steps > 1 && durationMs > 0) {
                Sleep(durationMs / steps);
            }
        }
    }

    void AntiAFKEngine::RestoreUserWindow(HWND prevWnd, int method) {
        if (method == 1) RestoreForegroundWindow(prevWnd);
        else if (method == 2) RestorePreviousWindowWithAltTab();
    }

    void AntiAFKEngine::RestoreForegroundWindow(HWND prevWnd) {
        if (!prevWnd || !IsWindow(prevWnd) || !IsWindowVisible(prevWnd) || IsIconic(prevWnd)) return;
        
        wchar_t className[256];
        GetClassNameW(prevWnd, className, 256);
        if (_wcsicmp(className, L"AntiAFK-HAB-tray") == 0) return;

        SetWindowPos(prevWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        DWORD curTh = GetCurrentThreadId();
        DWORD prevTh = GetWindowThreadProcessId(prevWnd, nullptr);
        AttachThreadInput(curTh, prevTh, TRUE);
        BringWindowToTop(prevWnd);
        SetForegroundWindow(prevWnd);
        AttachThreadInput(curTh, prevTh, FALSE);
        SetWindowPos(prevWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    void AntiAFKEngine::RestorePreviousWindowWithAltTab() {
        InputSimulator::SendKey(VK_MENU, false);
        InputSimulator::SendKeyPress(VK_TAB, 15);
        InputSimulator::SendKey(VK_MENU, true);
    }

    void AntiAFKEngine::EngineLoop(HWND hwndOwner) {
        while (m_running) {
            std::unique_lock<std::mutex> lock(m_mutex);
            HWND prevWnd = GetForegroundWindow();
            auto wins = RobloxDetector::FindAllRobloxWindows(true, m_settings->fishstrapSupport);

            if (wins.empty()) {
                // Roblox tiba-tiba tertutup saat mesin sedang aktif
                if (m_settings->statusBarEnabled) {
                    UI::StatusBarOverlay::GetInstance().Show(L"Warning: Roblox closed/not found!", 2000, hwndOwner);
                }
                Network::DiscordWebhook::QueueEvent(Network::WebhookEvent::Error, *m_settings);

                // Jika fitur Auto-Start dinyalakan, matikan mesin otomatis
                if (m_settings->autoStartAfk) {
                    Stop(*m_settings);
                    break;
                }

                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            if (m_settings->userSafeMode > 0) {
                uint64_t waitStart = GetTickCount64();
                while (m_userActive.load() && m_running) {
                    if ((GetTickCount64() - waitStart) / 1000 >= 60) break; 
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }

            if (m_settings->statusBarEnabled) {
                UI::StatusBarOverlay::GetInstance().Show(L"Performing anti-AFK action...", 3000, hwndOwner);
            }

            bool wasPaused = FpsCapper::GetInstance().IsPaused();
            FpsCapper::GetInstance().Pause(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));

            bool reconnectPerformed = false;
            for (size_t i = 0; i < wins.size(); ++i) {
                if (!m_running) break;
                HWND w = wins[i];

                bool wasMinimized = IsIconic(w) != FALSE;
                if (wasMinimized) ShowWindow(w, SW_RESTORE);

                SetForegroundWindow(w);
                Sleep(200);

                if (m_settings->autoReconnect) {
                    if (CheckAndPerformReconnect(w)) {
                        m_settings->autoReconnects++;
                        reconnectPerformed = true;
                    }
                }

                ExecuteAntiIdleAction(w, m_settings->selectedAction);
                if (m_settings->autoReset) {
                    ExecuteAutoReset(w);
                }

                if (wasMinimized) ShowWindow(w, SW_MINIMIZE);

                if (m_settings->multiSupport && m_settings->multiInstanceInterval > 0 && i < wins.size() - 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(m_settings->multiInstanceInterval));
                }
            }

            FpsCapper::GetInstance().Pause(wasPaused);
            RestoreUserWindow(prevWnd, m_settings->restoreMethod);

            m_settings->afkActions++;
            
            if (m_running && m_settings->statusBarEnabled) {
                wchar_t actionDoneMsg[128];
                swprintf_s(actionDoneMsg, L"Action Completed! Next in %d min", m_settings->selectedTime / 60);
                UI::StatusBarOverlay::GetInstance().Show(actionDoneMsg, 2500, hwndOwner);
            }

            m_secondsRemaining = m_settings->selectedTime;
            while (m_secondsRemaining > 0 && m_running) {
                m_cv.wait_for(lock, std::chrono::seconds(1), [this]() {
                    return !m_running;
                });
                if (!m_running) break;
                m_secondsRemaining--;
            }
        }
    }

    void AntiAFKEngine::ForceTriggerAction(const HAB::Settings& settings) {
        auto wins = RobloxDetector::FindAllRobloxWindows(true, settings.fishstrapSupport);
        if (wins.empty()) return;

        std::thread([this, wins, settings]() {
            HWND prevWnd = GetForegroundWindow();
            bool wasPaused = FpsCapper::GetInstance().IsPaused();
            FpsCapper::GetInstance().Pause(true);

            for (HWND w : wins) {
                bool minimized = IsIconic(w) != FALSE;
                if (minimized) ShowWindow(w, SW_RESTORE);
                SetForegroundWindow(w);
                Sleep(200);
                ExecuteAntiIdleAction(w, settings.selectedAction);
                if (minimized) ShowWindow(w, SW_MINIMIZE);
            }

            FpsCapper::GetInstance().Pause(wasPaused);
            RestoreUserWindow(prevWnd, settings.restoreMethod);
        }).detach();
    }
}