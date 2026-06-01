#include "RobloxDetector.h"
#include <tlhelp32.h>
#include <algorithm>

namespace HAB::Core {
    HANDLE RobloxDetector::m_hMultiInstanceMutex = nullptr;

    BOOL CALLBACK RobloxDetector::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
        auto* data = reinterpret_cast<EnumData*>(lParam);
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == data->processId && GetWindowTextLengthW(hwnd) > 0) {
            if (data->includeHidden || IsWindowVisible(hwnd)) {
                data->windows->push_back(hwnd);
            }
        }
        return TRUE;
    }

    std::vector<HWND> RobloxDetector::FindAllRobloxWindows(bool includeHidden, bool fishstrapSupport) {
        std::vector<HWND> wins;
        std::vector<DWORD> pids = FindAllRobloxProcessIds(fishstrapSupport);

        for (DWORD pid : pids) {
            std::vector<HWND> procWins;
            EnumData data = { pid, includeHidden, &procWins };
            EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
            wins.insert(wins.end(), procWins.begin(), procWins.end());
        }
        return wins;
    }

    std::vector<DWORD> RobloxDetector::FindAllRobloxProcessIds(bool fishstrapSupport) {
        std::vector<DWORD> pids;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return pids;

        PROCESSENTRY32W pe = { 0 };
        pe.dwSize = sizeof(pe);

        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"RobloxPlayerBeta.exe") == 0 ||
                    (fishstrapSupport && _wcsicmp(pe.szExeFile, L"eurotrucks2.exe") == 0)) {
                    if (std::find(pids.begin(), pids.end(), pe.th32ProcessID) == pids.end()) {
                        pids.push_back(pe.th32ProcessID);
                    }
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return pids;
    }

    void RobloxDetector::EnableMultiInstance(bool enable) {
        if (enable) {
            if (!m_hMultiInstanceMutex) {
                m_hMultiInstanceMutex = CreateMutexW(nullptr, TRUE, L"ROBLOX_singletonEvent");
            }
        } else {
            if (m_hMultiInstanceMutex) {
                CloseHandle(m_hMultiInstanceMutex);
                m_hMultiInstanceMutex = nullptr;
            }
        }
    }

    bool RobloxDetector::IsMultiInstanceEnabled() {
        return m_hMultiInstanceMutex != nullptr;
    }

    void RobloxDetector::RefreshWindowOpacity(bool forceDisable, bool isSessionOpacityEnabled, bool autoOpacity, bool isRunning, bool fishstrapSupport) {
        auto wins = FindAllRobloxWindows(true, fishstrapSupport);
        bool shouldApply = !forceDisable && (isSessionOpacityEnabled || (autoOpacity && isRunning));
        
        for (HWND w : wins) {
            LONG exStyle = GetWindowLongW(w, GWL_EXSTYLE);
            if (shouldApply) {
                SetWindowLongW(w, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
                SetLayeredWindowAttributes(w, 0, 180, LWA_ALPHA);
            } else {
                SetWindowLongW(w, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
                RedrawWindow(w, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
            }
        }
    }

    void RobloxDetector::GridSnapRobloxWindows(bool fishstrapSupport) {
        auto allWins = FindAllRobloxWindows(true, fishstrapSupport);
        std::vector<HWND> wins;

        auto isCandidate = [](HWND w) -> bool {
            if (!IsWindow(w)) return false;
            if (GetWindow(w, GW_OWNER) != nullptr) return false;
            if (GetWindowLongW(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return false;

            wchar_t title[256] = { 0 };
            GetWindowTextW(w, title, ARRAYSIZE(title));
            if (title[0] == L'\0') return false;

            wchar_t className[128] = { 0 };
            GetClassNameW(w, className, ARRAYSIZE(className));
            if (_wcsicmp(className, L"IME") == 0 || _wcsicmp(className, L"MSCTFIME UI") == 0) return false;

            RECT rc = { 0 };
            if (!GetWindowRect(w, &rc)) return false;
            if ((rc.right - rc.left) < 220 || (rc.bottom - rc.top) < 160) return false;

            return true;
        };

        for (HWND w : allWins) {
            if (isCandidate(w)) {
                if (IsIconic(w)) ShowWindow(w, SW_RESTORE);
                else if (!IsWindowVisible(w)) ShowWindow(w, SW_SHOWNOACTIVATE);
                wins.push_back(w);
            }
        }

        if (wins.empty()) return;

        std::sort(wins.begin(), wins.end());
        wins.erase(std::unique(wins.begin(), wins.end()), wins.end());

        RECT workArea;
        if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)) {
            GetWindowRect(GetDesktopWindow(), &workArea);
        }

        int margin = 8;
        int gap = 14;
        int screenW = workArea.right - workArea.left;
        int screenH = workArea.bottom - workArea.top;
        int count = static_cast<int>(wins.size());

        int cols = 1;
        while (cols * cols < count) cols++;
        int rows = (count + cols - 1) / cols;

        int usableW = screenW - margin * 2;
        int usableH = screenH - margin * 2;
        int availW = usableW - (cols - 1) * gap;
        int availH = usableH - (rows - 1) * gap;

        if (availW <= 0 || availH <= 0) return;

        int winW = 800;
        int winH = 600;

        if (cols * winW > availW || rows * winH > availH) {
            double scaleW = static_cast<double>(availW) / (cols * 800.0);
            double scaleH = static_cast<double>(availH) / (rows * 600.0);
            double scale = (std::min)(scaleW, scaleH);
            if (scale <= 0.0) scale = 1.0;

            winW = (std::max)(220, static_cast<int>(800 * scale));
            winH = (std::max)(165, static_cast<int>(600 * scale));
        }

        int startX = workArea.left + margin;
        int startY = workArea.top + margin;

        for (int i = 0; i < count; ++i) {
            int col = i % cols;
            int row = i / cols;
            int x = startX + col * (winW + gap);
            int y = startY + row * (winH + gap);

            SetWindowPos(wins[i], nullptr, x, y, winW, winH, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
    }
}