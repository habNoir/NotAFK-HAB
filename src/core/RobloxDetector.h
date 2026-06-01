#pragma once
#include <windows.h>
#include <vector>

namespace HAB::Core {
    struct EnumData {
        DWORD processId;
        bool includeHidden;
        std::vector<HWND>* windows;
    };

    class RobloxDetector {
    public:
        static std::vector<HWND> FindAllRobloxWindows(bool includeHidden = false, bool fishstrapSupport = false);
        static std::vector<DWORD> FindAllRobloxProcessIds(bool fishstrapSupport = false);
        static void GridSnapRobloxWindows(bool fishstrapSupport = false);
        static void RefreshWindowOpacity(bool forceDisable, bool isSessionOpacityEnabled, bool autoOpacity, bool isRunning, bool fishstrapSupport);
        static void EnableMultiInstance(bool enable);
        static bool IsMultiInstanceEnabled();

    private:
        static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
        static HANDLE m_hMultiInstanceMutex;
    };
}