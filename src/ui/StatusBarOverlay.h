#pragma once
#include <windows.h>
#include <string>
#include <memory>

namespace HAB::UI {
    class StatusBarOverlay {
    public:
        static StatusBarOverlay& GetInstance();
        void Show(const std::wstring& message, UINT durationMs, HWND anchorWnd = nullptr);
        void Hide(bool animate = true);
        void Initialize(HINSTANCE hInst);

    private:
        StatusBarOverlay() = default;
        ~StatusBarOverlay();

        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void OnPaint(HWND hwnd);
        void OnTimer(HWND hwnd, UINT_PTR timerId);

        HWND m_hwnd = nullptr;
        HINSTANCE m_hInst = nullptr;
        std::wstring m_message = L"Ready";
        float m_currentAlpha = 0.0f;
        float m_targetAlpha = 0.0f;
        RECT m_targetBounds = { 0 };
    };
}