#include "StatusBarOverlay.h"
#include "UIStyle.h"
#include "AnimationHelper.h"
#include <dwmapi.h>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")

// Import namespace agar semua fungsi style/warna HAB bisa diakses langsung
using namespace HAB::UI;
// Import namespace Gdiplus agar tipe data REAL, PointF, dll bisa dikenali langsung
using namespace Gdiplus; 

namespace HAB::UI {
    constexpr UINT_PTR TIMER_HIDE = 1;
    constexpr UINT_PTR TIMER_ANIM = 2;

    StatusBarOverlay& StatusBarOverlay::GetInstance() {
        static StatusBarOverlay instance;
        return instance;
    }

    StatusBarOverlay::~StatusBarOverlay() {
        if (m_hwnd) DestroyWindow(m_hwnd);
    }

    void StatusBarOverlay::Initialize(HINSTANCE hInst) {
        m_hInst = hInst;
        const wchar_t CLASS_NAME[] = L"AntiAFK-HAB-StatusBar";

        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInst;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        RegisterClassW(&wc);
    }

    void StatusBarOverlay::Show(const std::wstring& message, UINT durationMs, HWND anchorWnd) {
        if (!m_hwnd) {
            m_hwnd = CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
                L"AntiAFK-HAB-StatusBar", L"", WS_POPUP,
                0, 0, 360, 40, nullptr, nullptr, m_hInst, nullptr
            );

            // Bikin rounded corner modern di Windows 11
            enum DWM_WINDOW_CORNER_PREFERENCE { DWMWCP_ROUND = 2 };
            const DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
            DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;
            DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
        }

        m_message = message;
        m_targetAlpha = 245.0f; // Sedikit transparan

        // Atur posisi tengah atas layar
        MONITORINFO mi = { sizeof(mi) };
        HMONITOR hMon = MonitorFromWindow(anchorWnd ? anchorWnd : GetForegroundWindow(), MONITOR_DEFAULTTONEAREST);
        GetMonitorInfoW(hMon, &mi);

        int scrW = mi.rcWork.right - mi.rcWork.left;
        int width = 340;
        int height = 36;
        int x = mi.rcWork.left + (scrW - width) / 2;
        int y = mi.rcWork.top + 12;

        m_targetBounds = { x, y, x + width, y + height };
        SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_NOOWNERZORDER);

        // Bikin region rounded untuk kompabilitas OS lama
        HRGN rgn = CreateRoundRectRgn(0, 0, width, height, 12, 12);
        SetWindowRgn(m_hwnd, rgn, TRUE);

        KillTimer(m_hwnd, TIMER_HIDE);
        SetTimer(m_hwnd, TIMER_HIDE, durationMs, nullptr);
        SetTimer(m_hwnd, TIMER_ANIM, 16, nullptr); // Loop Animasi 60 FPS

        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void StatusBarOverlay::Hide(bool animate) {
        if (!m_hwnd) return;
        KillTimer(m_hwnd, TIMER_HIDE);
        if (animate) {
            m_targetAlpha = 0.0f;
        } else {
            m_currentAlpha = 0.0f;
            m_targetAlpha = 0.0f;
            SetLayeredWindowAttributes(m_hwnd, 0, 0, LWA_ALPHA);
            ShowWindow(m_hwnd, SW_HIDE);
        }
    }

    LRESULT CALLBACK StatusBarOverlay::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_PAINT:
                GetInstance().OnPaint(hwnd);
                break;
            case WM_TIMER:
                GetInstance().OnTimer(hwnd, wParam);
                break;
            case WM_NCHITTEST:
                return HTTRANSPARENT; // Supaya tembus klik mouse
            default:
                return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void StatusBarOverlay::OnTimer(HWND hwnd, UINT_PTR timerId) {
        if (timerId == TIMER_HIDE) {
            KillTimer(hwnd, TIMER_HIDE);
            Hide(true);
        } else if (timerId == TIMER_ANIM) {
            float prevAlpha = m_currentAlpha;
            m_currentAlpha = AnimationHelper::Lerp(m_currentAlpha, m_targetAlpha, 0.15f);

            if (std::abs(m_currentAlpha - prevAlpha) > 0.1f) {
                SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(m_currentAlpha), LWA_ALPHA);
                InvalidateRect(hwnd, nullptr, FALSE);
            } else if (m_targetAlpha == 0.0f && m_currentAlpha < 1.0f) {
                KillTimer(hwnd, TIMER_ANIM);
                ShowWindow(hwnd, SW_HIDE);
            }
        }
    }

    void StatusBarOverlay::OnPaint(HWND hwnd) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        // Double Buffering (Cegah Flicker)
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        // Render Background Panel HAB Premium
        SolidBrush bgBrush(GetGdiColor(COLOR_BG_DARK, 255));
        Pen borderPen(GetGdiColor(COLOR_ACCENT_PRIMARY, 180), 1.5f);

        Gdiplus::GraphicsPath path;
        REAL radius = 8.0f;
        path.AddArc(RectF(0, 0, radius * 2, radius * 2), 180, 90);
        path.AddArc(RectF(w - (radius * 2) - 1, 0, radius * 2, radius * 2), 270, 90);
        path.AddArc(RectF(w - (radius * 2) - 1, h - (radius * 2) - 1, radius * 2, radius * 2), 0, 90);
        path.AddArc(RectF(0, h - (radius * 2) - 1, radius * 2, radius * 2), 90, 90);
        path.CloseFigure();

        g.FillPath(&bgBrush, &path);
        g.DrawPath(&borderPen, &path);

        // Render Ikon Info (MDL2)
        Font iconFont(L"Segoe MDL2 Assets", 10.0f);
        SolidBrush accentBrush(GetGdiColor(COLOR_ACCENT_SECONDARY));
        g.DrawString(L"\uE946", -1, &iconFont, PointF(14.0f, 11.0f), &accentBrush);

        // Render Judul Program
        Font titleFont(L"Segoe UI", 9.0f, FontStyleBold);
        SolidBrush textWhite(GetGdiColor(COLOR_TEXT_LIGHT));
        g.DrawString(L"Anti-AFK HAB", -1, &titleFont, PointF(34.0f, 9.0f), &textWhite);

        // Render Separator Dot
        Font normalFont(L"Segoe UI", 9.0f);
        SolidBrush textMuted(GetGdiColor(COLOR_TEXT_MUTED));
        g.DrawString(L"\u2022", -1, &normalFont, PointF(122.0f, 9.0f), &textMuted);

        // Render Status Message
        g.DrawString(m_message.c_str(), -1, &normalFont, PointF(138.0f, 9.0f), &textMuted);

        // Blit ke layar asli
        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
    }
}