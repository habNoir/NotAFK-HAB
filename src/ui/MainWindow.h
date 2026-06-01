#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include "../utils/ConfigManager.h"

namespace HAB::UI {
    struct TabItem {
        std::wstring name;
        std::wstring iconGlyph;
        RECT bounds;
        float currentWidth; 
    };

    class MainWindow {
    public:
        static MainWindow& GetInstance();
        void Initialize(HINSTANCE hInst, HAB::Settings& settings);
        void Show();

    private:
        MainWindow();
        ~MainWindow();

        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void OnPaint(HWND hwnd);
        void OnMouseMove(HWND hwnd, int x, int y);
        void OnLButtonDown(HWND hwnd, int x, int y);
        void OnLButtonUp(HWND hwnd);
        void OnTimer(HWND hwnd, UINT_PTR timerId);

        // Fungsi Inline Translation Helper kustom HAB
        std::wstring L(const std::wstring& en, const std::wstring& id) const {
            return (m_settings && m_settings->language == 1) ? id : en;
        }

        void DrawToggleRow(Gdiplus::Graphics& g, Gdiplus::Font& font, int index, const std::wstring& icon, const std::wstring& label, bool checked, float animValue, float rowY);
        void DrawSelectorRow(Gdiplus::Graphics& g, Gdiplus::Font& font, int index, const std::wstring& icon, const std::wstring& label, const std::wstring& value, float rowY);

        HWND m_hwnd = nullptr;
        HINSTANCE m_hInst = nullptr;
        HAB::Settings* m_settings = nullptr;

        HWND m_hEditWebhook = nullptr;
        HBRUSH m_hEditBrush = nullptr;

        int m_activeTab = 0;
        float m_navIndicatorX = 0.0f;
        float m_navIndicatorW = 0.0f;
        float m_startBtnScale = 1.0f;
        bool m_isHoveringStart = false;
        bool m_isPressingStart = false;

        float m_multiSupportAnim = 0.0f;
        float m_autoStartAnim = 0.0f;
        float m_autoReconnectAnim = 0.0f;
        float m_autoResetAnim = 0.0f;
        float m_autoHideAnim = 0.0f;
        float m_unlockFpsAnim = 0.0f;
        float m_autoMuteAnim = 0.0f;

        std::vector<TabItem> m_tabs;
        RECT m_startBtnRect = { 0 };
    };
}