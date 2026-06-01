#include "InputSimulator.h"

namespace HAB::Core {
    void InputSimulator::SendKey(WORD vkCode, bool isKeyUp) {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.wScan = static_cast<WORD>(MapVirtualKey(vkCode, 0));
        input.ki.dwFlags = (isKeyUp ? KEYEVENTF_KEYUP : 0) | KEYEVENTF_SCANCODE;
        
        SendInput(1, &input, sizeof(INPUT));
    }

    void InputSimulator::SendKeyPress(WORD vkCode, DWORD durationMs) {
        SendKey(vkCode, false);
        Sleep(durationMs);
        SendKey(vkCode, true);
    }

    void InputSimulator::MoveMouseRelative(int dx, int dy) {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = dx;
        input.mi.dy = dy;
        SendInput(1, &input, sizeof(INPUT));
    }

    void InputSimulator::PerformClick() {
        INPUT input[2] = { 0 };
        input[0].type = INPUT_MOUSE;
        input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        
        SendInput(2, input, sizeof(INPUT));
    }
}