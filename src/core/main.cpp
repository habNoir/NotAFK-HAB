#include <windows.h>
#include <gdiplus.h>
#include <dwmapi.h> 
#include "../utils/ConfigManager.h"
#include "AntiAFKEngine.h"
#include "FpsCapper.h"
#include "RobloxDetector.h"
#include "../ui/MainWindow.h"
#include "../ui/StatusBarOverlay.h"
#include "../ui/UIStyle.h" 
#include "../../resource.h" 

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(linker, "/subsystem:windows")

// Menggunakan namespace untuk mempermudah akses fungsi visual kustom
using namespace Gdiplus;
using namespace HAB::UI;

#define ID_TRAY_ICON 1
#define ID_TRAY_OPEN 1001
#define ID_TRAY_EXIT 1002

// Deklarasi Variabel Global diletakkan di atas agar bisa diakses seluruh fungsi di bawahnya
NOTIFYICONDATAW g_nid = { 0 };
HWND g_hTrayWnd = nullptr;
HMENU g_hTrayMenu = nullptr;
HAB::Settings g_settings;

// Fungsi untuk Memperbarui Ikon Baki secara Dinamis Berdasarkan Status Mesin
void UpdateTrayIcon(bool isRunning, bool isMulti) {
    if (!g_hTrayWnd) return;

    int iconId = IDI_TRAY_OFF;
    if (isRunning) {
        iconId = isMulti ? IDI_TRAY_ON_MULTI : IDI_TRAY_ON;
    }

    g_nid.hIcon = LoadIconW(GetModuleHandle(nullptr), MAKEINTRESOURCEW(iconId));
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// --- WINDOW PROC UNTUK SPLASH SCREEN (HAB PREMIUM SPLASH) ---
LRESULT CALLBACK SplashWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            // Double buffering untuk mencegah flicker (kedipan layar)
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
            HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

            Graphics g(memDC);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            // Mewarnai background gelap Obsidian
            SolidBrush bgBrush(GetGdiColor(COLOR_BG_DARK));
            g.FillRectangle(&bgBrush, 0, 0, w, h);

            // Gambar Ikon Tengah (Besar)
            HICON hAppIcon = LoadIconW(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_MAIN));
            if (hAppIcon) {
                DrawIconEx(memDC, (w - 48) / 2, h / 2 - 50, hAppIcon, 48, 48, 0, nullptr, DI_NORMAL);
            }

            // Gambar Nama Aplikasi HAB
            Font titleFont(L"Segoe UI", 16.0f, FontStyleBold);
            SolidBrush textWhite(GetGdiColor(COLOR_TEXT_LIGHT));
            StringFormat sf;
            sf.SetAlignment(StringAlignmentCenter);
            g.DrawString(L"Anti-AFK HAB", -1, &titleFont, RectF(0.0f, h / 2.0f + 16.0f, static_cast<float>(w), 30.0f), &sf, &textWhite);

            // Gambar Versi Aplikasi & Status Loading
            Font subFont(L"Segoe UI", 9.0f);
            SolidBrush textMuted(GetGdiColor(COLOR_TEXT_MUTED));
            g.DrawString(L"v1.0 • Loading system...", -1, &subFont, RectF(0.0f, h / 2.0f + 50.0f, static_cast<float>(w), 20.0f), &sf, &textMuted);

            BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_TIMER:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Fungsi untuk Membuat dan Menampilkan Splash Screen Selama 1.5 Detik
void ShowSplashScreen(HINSTANCE hInst) {
    const wchar_t CLASS_NAME[] = L"AntiAFK-HAB-Splash";

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = SplashWindowProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassW(&wc);

    int winW = 320;
    int winH = 220;
    int x = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - winH) / 2;

    HWND hSplash = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"", WS_POPUP,
        x, y, winW, winH, nullptr, nullptr, hInst, nullptr
    );

    if (hSplash) {
        // Pasang sudut membulat modern untuk Windows 11
        enum DWM_WINDOW_CORNER_PREFERENCE { DWMWCP_ROUND = 2 };
        const DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
        DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;
        DwmSetWindowAttribute(hSplash, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));

        SetTimer(hSplash, 1, 1500, nullptr); // Hancurkan otomatis setelah 1.5 detik
        ShowWindow(hSplash, SW_SHOW);
        UpdateWindow(hSplash);

        MSG msg;
        while (IsWindow(hSplash) && GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

// --- WINDOW PROC TRAY ICON ---
LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_nid.cbSize = sizeof(NOTIFYICONDATAW);
            g_nid.hWnd = hwnd;
            g_nid.uID = ID_TRAY_ICON;
            g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            g_nid.uCallbackMessage = WM_USER + 1;
            g_nid.hIcon = LoadIconW(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_TRAY_OFF));
            lstrcpyW(g_nid.szTip, L"Anti-AFK HAB v1.0");
            Shell_NotifyIconW(NIM_ADD, &g_nid);
            break;

        case WM_USER + 1:
            if (lParam == WM_RBUTTONDOWN) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);

                if (g_hTrayMenu) DestroyMenu(g_hTrayMenu);
                g_hTrayMenu = CreatePopupMenu();
                AppendMenuW(g_hTrayMenu, MF_STRING, ID_TRAY_OPEN, L"Open Anti-AFK HAB");
                AppendMenuW(g_hTrayMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(g_hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit App");

                TrackPopupMenu(g_hTrayMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            } else if (lParam == WM_LBUTTONDOWN) {
                HAB::UI::MainWindow::GetInstance().Show();
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_OPEN) {
                HAB::UI::MainWindow::GetInstance().Show();
            } else if (LOWORD(wParam) == ID_TRAY_EXIT) {
                PostQuitMessage(0);
            }
            break;

        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // 1. Inisialisasi GDI+ Graphics
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // 2. Memuat Konfigurasi Pengaturan HAB
    HAB::ConfigManager::Load(g_settings);
    g_settings.programLaunches++;
    HAB::ConfigManager::Save(g_settings);

    // 3. Tampilkan Jendela Splash Screen HAB (1.5 detik)
    ShowSplashScreen(hInstance);

    // 4. Deteksi Roblox & Batas Multi-Instance
    HAB::Core::RobloxDetector::EnableMultiInstance(g_settings.multiSupport);

    // 5. Inisialisasi UI Utama & Jendela Status Bar
    HAB::UI::MainWindow::GetInstance().Initialize(hInstance, g_settings);
    HAB::UI::StatusBarOverlay::GetInstance().Initialize(hInstance);

    // 6. Jalankan Pembatas FPS jika Diaktifkan
    if (g_settings.fpsLimit > 0) {
        HAB::Core::FpsCapper::GetInstance().Start(g_settings.fpsLimit, g_settings.unlockFpsOnFocus, g_settings.fishstrapSupport);
    }

    // 7. Daftarkan Jendela Baki Sistem (Tray Icon)
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = TrayWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AntiAFK-HAB-tray";
    RegisterClassW(&wc);

    g_hTrayWnd = CreateWindowExW(0, L"AntiAFK-HAB-tray", L"", WS_OVERLAPPEDWINDOW,
        0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

    // Tampilkan Antarmuka Jendela Utama secara Default
    HAB::UI::MainWindow::GetInstance().Show();

    // 8. Siklus Message Loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 9. Tutup Seluruh Thread Mesin & FPS Capper secara Aman Sebelum Keluar
    HAB::Core::AntiAFKEngine::GetInstance().Stop(g_settings);
    HAB::Core::FpsCapper::GetInstance().Stop();

    DestroyWindow(g_hTrayWnd);
    GdiplusShutdown(gdiplusToken);
    return 0;
}