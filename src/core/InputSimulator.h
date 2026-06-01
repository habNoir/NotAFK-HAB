#pragma once
#include <windows.h>

namespace HAB::Core {
    class InputSimulator {
    public:
        static void SendKey(WORD vkCode, bool isKeyUp = false);
        static void SendKeyPress(WORD vkCode, DWORD durationMs = 15);
        static void MoveMouseRelative(int dx, int dy);
        static void PerformClick();
    };
}