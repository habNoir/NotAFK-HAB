#include "MainWindow.h"
#include "UIStyle.h"
#include "AnimationHelper.h"
#include "../core/AntiAFKEngine.h"
#include "../core/FpsCapper.h"
#include "../core/RobloxDetector.h"
#include "StatusBarOverlay.h"
#include "../network/DiscordWebhook.h"
#include <dwmapi.h>
#include <commctrl.h>

using namespace HAB::UI;
using namespace Gdiplus;

extern void UpdateTrayIcon(bool isRunning, bool isMulti);

namespace HAB::UI {
    constexpr UINT_PTR TIMER_UI_REFRESH = 1;
    constexpr UINT IDC_EDIT_WEBHOOK = 1020;

    MainWindow::MainWindow() {
        m_hwnd = nullptr;
        m_hInst = nullptr;
        m_settings = nullptr;
        m_hEditWebhook = nullptr;
        m_hEditBrush = nullptr;
    }

    MainWindow& MainWindow::GetInstance() {
        static MainWindow instance;
        return instance;
    }

    MainWindow::~MainWindow() {
        if (m_hwnd) DestroyWindow(m_hwnd);
        if (m_hEditBrush) DeleteObject(m_hEditBrush);
    }

    std::wstring GetClipboardText(HWND hwnd) {
        std::wstring text = L"";
        if (OpenClipboard(hwnd)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                wchar_t* pText = static_cast<wchar_t*>(GlobalLock(hData));
                if (pText) {
                    text = pText;
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
        return text;
    }

    void MainWindow::Initialize(HINSTANCE hInst, Settings& settings) {
        m_hInst = hInst;
        m_settings = &settings;
        const wchar_t CLASS_NAME[] = L"AntiAFK-HAB-MainUI";

        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInst;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;

        RegisterClassW(&wc);

        m_activeTab = 0;
        m_navIndicatorX = 0.0f;
        m_navIndicatorW = 0.0f;
        m_startBtnScale = 1.0f;

        // Perbaikan kurung pembuka L( yang sempat terhapus
        m_tabs = {
            { L(L"General",  L"Umum"),      L"\xE80F", { 0, 0, 0, 0 }, 50.0f },
            { L(L"Auto",     L"Otomatis"),  L"\xE945", { 0, 0, 0, 0 }, 50.0f },
            { L(L"Stats",    L"Statistik"), L"\xE9D2", { 0, 0, 0, 0 }, 50.0f },
            { L(L"Advanced", L"Lanjutan"),  L"\xE713", { 0, 0, 0, 0 }, 50.0f },
            { L(L"Utils",    L"Alat"),      L"\xE7FC", { 0, 0, 0, 0 }, 50.0f },
            { L(L"Webhook",  L"Webhook"),   L"\xE8BD", { 0, 0, 0, 0 }, 50.0f }
        };

        m_hEditBrush = CreateSolidBrush(COLOR_CARD_DARK);
    }

    void MainWindow::Show() {
        if (!m_hwnd) {
            int winW = 380;
            int winH = 460;
            int x = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
            int y = (GetSystemMetrics(SM_CYSCREEN) - winH) / 2;

            m_hwnd = CreateWindowExW(
                WS_EX_APPWINDOW, L"AntiAFK-HAB-MainUI",
                L"Anti-AFK HAB v1.0", WS_POPUP,
                x, y, winW, winH, nullptr, nullptr, m_hInst, nullptr
            );

            m_hEditWebhook = CreateWindowExW(
                0, L"EDIT", L"",
                WS_CHILD | ES_AUTOHSCROLL | ES_LEFT,
                34, 142, 312, 22, m_hwnd, (HMENU)IDC_EDIT_WEBHOOK, m_hInst, nullptr
            );

            HFONT hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            SendMessageW(m_hEditWebhook, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Tambahkan .c_str() untuk mengonversi std::wstring ke LPARAM (const wchar_t*)
            SendMessageW(m_hEditWebhook, EM_SETCUEBANNER, FALSE, (LPARAM)L(L"Type or paste Webhook URL here...", L"Ketik atau tempel URL Webhook di sini...").c_str());

            SetWindowTextW(m_hEditWebhook, m_settings->discordWebhookUrl.c_str());

            enum DWM_WINDOW_CORNER_PREFERENCE { DWMWCP_ROUND = 2 };
            const DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
            DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;
            DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));

            SetTimer(m_hwnd, TIMER_UI_REFRESH, 16, nullptr);
        }
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }

    LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_PAINT:
                GetInstance().OnPaint(hwnd);
                break;
            case WM_MOUSEMOVE:
                GetInstance().OnMouseMove(hwnd, LOWORD(lParam), HIWORD(lParam));
                break;
            case WM_LBUTTONDOWN:
                GetInstance().OnLButtonDown(hwnd, LOWORD(lParam), HIWORD(lParam));
                break;
            case WM_LBUTTONUP:
                GetInstance().OnLButtonUp(hwnd);
                break;
            case WM_TIMER:
                GetInstance().OnTimer(hwnd, wParam);
                break;

            case WM_CTLCOLOREDIT: {
                HDC hdcEdit = (HDC)wParam;
                SetTextColor(hdcEdit, COLOR_TEXT_LIGHT);
                SetBkColor(hdcEdit, COLOR_CARD_DARK);
                return (LRESULT)GetInstance().m_hEditBrush;
            }

            case WM_COMMAND: {
                if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT_WEBHOOK) {
                    wchar_t buf[2048] = { 0 };
                    GetWindowTextW(GetInstance().m_hEditWebhook, buf, 2048);
                    GetInstance().m_settings->discordWebhookUrl = buf;
                    ConfigManager::Save(*GetInstance().m_settings);
                }
                break;
            }

            case WM_NCHITTEST: {
                POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                ScreenToClient(hwnd, &pt);
                if (pt.y < 35 && pt.x < 340) return HTCAPTION;
                return HTCLIENT;
            }
            case WM_CLOSE:
                ShowWindow(hwnd, SW_HIDE);
                break;
            default:
                return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void MainWindow::OnTimer(HWND hwnd, UINT_PTR timerId) {
        if (timerId == TIMER_UI_REFRESH) {
            bool needsRedraw = false;

            if (m_activeTab < static_cast<int>(m_tabs.size())) {
                const RECT& r = m_tabs[m_activeTab].bounds;
                float targetX = static_cast<float>(r.left);
                float targetW = static_cast<float>(r.right - r.left);

                float prevX = m_navIndicatorX;
                float prevW = m_navIndicatorW;

                m_navIndicatorX = AnimationHelper::Lerp(m_navIndicatorX, targetX, 0.2f);
                m_navIndicatorW = AnimationHelper::Lerp(m_navIndicatorW, targetW, 0.2f);

                if (std::abs(m_navIndicatorX - prevX) > 0.1f || std::abs(m_navIndicatorW - prevW) > 0.1f) {
                    needsRedraw = true;
                }
            }

            float targetScale = m_isPressingStart ? 0.94f : (m_isHoveringStart ? 1.02f : 1.00f);
            float prevScale = m_startBtnScale;
            m_startBtnScale = AnimationHelper::Lerp(m_startBtnScale, targetScale, 0.15f);
            if (std::abs(m_startBtnScale - prevScale) > 0.005f) {
                needsRedraw = true;
            }

            auto updateAnim = [](float& val, bool target, bool& redraw) {
                float targetVal = target ? 1.0f : 0.0f;
                float prev = val;
                val = AnimationHelper::Lerp(val, targetVal, 0.2f);
                if (std::abs(val - prev) > 0.01f) redraw = true;
            };

            updateAnim(m_multiSupportAnim, m_settings->multiSupport, needsRedraw);
            updateAnim(m_autoStartAnim, m_settings->autoStartAfk, needsRedraw);
            updateAnim(m_autoReconnectAnim, m_settings->autoReconnect, needsRedraw);
            updateAnim(m_autoResetAnim, m_settings->autoReset, needsRedraw);
            updateAnim(m_autoHideAnim, m_settings->autoHideRoblox, needsRedraw);
            updateAnim(m_unlockFpsAnim, m_settings->unlockFpsOnFocus, needsRedraw);
            updateAnim(m_autoMuteAnim, m_settings->autoMute, needsRedraw);

            if (Core::AntiAFKEngine::GetInstance().IsRunning()) {
                needsRedraw = true;
            }

            if (needsRedraw) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
    }

    void MainWindow::OnMouseMove(HWND hwnd, int x, int y) {
        POINT pt = { x, y };
        bool nowHoverStart = PtInRect(&m_startBtnRect, pt) != FALSE;
        if (nowHoverStart != m_isHoveringStart) {
            m_isHoveringStart = nowHoverStart;
            InvalidateRect(hwnd, &m_startBtnRect, FALSE);
        }
    }

    void MainWindow::OnLButtonDown(HWND hwnd, int x, int y) {
        POINT pt = { x, y };
        
        if (PtInRect(&m_startBtnRect, pt)) {
            m_isPressingStart = true;
            InvalidateRect(hwnd, &m_startBtnRect, FALSE);
            return;
        }

        for (size_t i = 0; i < m_tabs.size(); ++i) {
            if (PtInRect(&m_tabs[i].bounds, pt)) {
                m_activeTab = static_cast<int>(i);
                
                if (m_hEditWebhook) {
                    if (m_activeTab == 5) {
                        SetWindowTextW(m_hEditWebhook, m_settings->discordWebhookUrl.c_str());
                        ShowWindow(m_hEditWebhook, SW_SHOW);
                    } else {
                        ShowWindow(m_hEditWebhook, SW_HIDE);
                    }
                }

                InvalidateRect(hwnd, nullptr, FALSE);
                return;
            }
        }

        // DETEKSI KLIK BARIS PRESEISI TANPA OFFSET
        if (x >= 16 && x <= 364 && y >= 92 && y <= (92 + 4 * 44)) {
            int rowIndex = (y - 92) / 44;
            int rowOffset = (y - 92) % 44;

            if (rowOffset <= 36) { 
                if (m_activeTab == 0) { // --- GENERAL PAGE ---
                    if (rowIndex == 0) { 
                        m_settings->multiSupport = !m_settings->multiSupport;
                        Core::RobloxDetector::EnableMultiInstance(m_settings->multiSupport);
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(m_settings->multiSupport ? L(L"Multi-Instance Support: ON", L"Dukungan Multi-Instance: AKTIF") : L(L"Multi-Instance Support: OFF", L"Dukungan Multi-Instance: MATI"), 1500, m_hwnd);
                    } 
                    else if (rowIndex == 1) { // Popup List Menu: Interval
                        HMENU hMenu = CreatePopupMenu();
                        AppendMenuW(hMenu, MF_STRING, 1, L"3 Minutes (180s)");
                        AppendMenuW(hMenu, MF_STRING, 2, L"6 Minutes (360s)");
                        AppendMenuW(hMenu, MF_STRING, 3, L"9 Minutes (540s)");
                        AppendMenuW(hMenu, MF_STRING, 4, L"12 Minutes (720s)");
                        AppendMenuW(hMenu, MF_STRING, 5, L"15 Minutes (900s)");

                        POINT menuPt;
                        GetCursorPos(&menuPt);
                        int selection = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN, menuPt.x, menuPt.y, 0, hwnd, nullptr);
                        DestroyMenu(hMenu);

                        if (selection > 0) {
                            m_settings->selectedTime = selection * 180;
                            ConfigManager::Save(*m_settings);
                            
                            wchar_t notifyBuf[128];
                            // Ditambahkan .c_str() untuk memproses string lokalisasi
                            swprintf_s(notifyBuf, L(L"Interval set to %d minutes", L"Jeda disetel ke %d menit").c_str(), selection * 3);
                            StatusBarOverlay::GetInstance().Show(notifyBuf, 1500, m_hwnd);
                        }
                    } 
                    else if (rowIndex == 2) { // Popup List Menu: AFK Action
                        HMENU hMenu = CreatePopupMenu();
                        AppendMenuW(hMenu, MF_STRING, 1, L"Space (Jump)");
                        AppendMenuW(hMenu, MF_STRING, 2, L"W/S Keys");
                        AppendMenuW(hMenu, MF_STRING, 3, L"Zoom I/O");

                        POINT menuPt;
                        GetCursorPos(&menuPt);
                        int selection = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN, menuPt.x, menuPt.y, 0, hwnd, nullptr);
                        DestroyMenu(hMenu);

                        if (selection > 0) {
                            m_settings->selectedAction = selection - 1;
                            ConfigManager::Save(*m_settings);

                            const wchar_t* actionNames[] = { L"Space (Jump)", L"W/S Keys", L"Zoom I/O" };
                            wchar_t notifyBuf[128];
                            // Ditambahkan .c_str() untuk memproses string lokalisasi
                            swprintf_s(notifyBuf, L(L"Action set to %s", L"Tindakan disetel ke %s").c_str(), actionNames[selection - 1]);
                            StatusBarOverlay::GetInstance().Show(notifyBuf, 1500, m_hwnd);
                        }
                    }
                } 
                else if (m_activeTab == 1) { // --- AUTO PAGE ---
                    if (rowIndex == 0) {
                        m_settings->autoStartAfk = !m_settings->autoStartAfk;
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(m_settings->autoStartAfk ? L(L"Auto-Start AFK: Enabled", L"Mulai AFK Otomatis: AKTIF") : L(L"Auto-Start AFK: Disabled", L"Mulai AFK Otomatis: MATI"), 1500, m_hwnd);
                    } else if (rowIndex == 1) {
                        m_settings->autoReconnect = !m_settings->autoReconnect;
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(m_settings->autoReconnect ? L(L"Auto-Reconnect: Enabled", L"Penyambung Otomatis: AKTIF") : L(L"Auto-Reconnect: Disabled", L"Penyambung Otomatis: MATI"), 1500, m_hwnd);
                    } else if (rowIndex == 2) {
                        m_settings->autoReset = !m_settings->autoReset;
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(m_settings->autoReset ? L(L"Auto-Reset Player: Enabled", L"Reset Player Otomatis: AKTIF") : L(L"Auto-Reset Player: Disabled", L"Reset Player Otomatis: MATI"), 1500, m_hwnd);
                    } else if (rowIndex == 3) {
                        m_settings->autoHideRoblox = !m_settings->autoHideRoblox;
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(m_settings->autoHideRoblox ? L(L"Auto-Hide Roblox: Enabled", L"Sembunyikan Roblox Otomatis: AKTIF") : L(L"Auto-Hide Roblox: Disabled", L"Sembunyikan Roblox Otomatis: MATI"), 1500, m_hwnd);
                    }
                }
                else if (m_activeTab == 2) { // --- STATS PAGE ---
                    if (rowIndex == 3) { // Reset Statistics
                        m_settings->afkActions = 0;
                        m_settings->autoReconnects = 0;
                        m_settings->totalAfkTime = 0;
                        m_settings->longestAfkSession = 0;
                        m_settings->afkSessionsCompleted = 0;
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(L(L"Statistics Reset Successfully", L"Statistik Berhasil Direset"), 1500, m_hwnd);
                    }
                }
                else if (m_activeTab == 3) { // --- ADVANCED PAGE ---
                    if (rowIndex == 0) {
                        m_settings->unlockFpsOnFocus = !m_settings->unlockFpsOnFocus;
                        Core::FpsCapper::GetInstance().SetUnlockOnFocus(m_settings->unlockFpsOnFocus);
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(m_settings->unlockFpsOnFocus ? L(L"Unlock FPS on Focus: ON", L"Buka Kunci FPS saat Fokus: AKTIF") : L(L"Unlock FPS on Focus: OFF", L"Buka Kunci FPS saat Fokus: MATI"), 1500, m_hwnd);
                    } else if (rowIndex == 1) {
                        m_settings->autoMute = !m_settings->autoMute;
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(m_settings->autoMute ? L(L"AutoMute: Enabled", L"Bisukan Roblox Otomatis: AKTIF") : L(L"AutoMute: Disabled", L"Bisukan Roblox Otomatis: MATI"), 1500, m_hwnd);
                    } else if (rowIndex == 2) { // Popup List Menu: FPS Capper
                        HMENU hMenu = CreatePopupMenu();
                        AppendMenuW(hMenu, MF_STRING, 1, L"Off (No Limit)");
                        AppendMenuW(hMenu, MF_STRING, 2, L"3 FPS (Background-Safe)");
                        AppendMenuW(hMenu, MF_STRING, 3, L"5 FPS");
                        AppendMenuW(hMenu, MF_STRING, 4, L"15 FPS");
                        AppendMenuW(hMenu, MF_STRING, 5, L"30 FPS");
                        AppendMenuW(hMenu, MF_STRING, 6, L"60 FPS");

                        POINT menuPt;
                        GetCursorPos(&menuPt);
                        int selection = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN, menuPt.x, menuPt.y, 0, hwnd, nullptr);
                        DestroyMenu(hMenu);

                        if (selection > 0) {
                            int limits[] = { 0, 0, 3, 5, 15, 30, 60 };
                            m_settings->fpsLimit = limits[selection];
                            
                            Core::FpsCapper::GetInstance().Stop();
                            if (m_settings->fpsLimit > 0) {
                                Core::FpsCapper::GetInstance().Start(m_settings->fpsLimit, m_settings->unlockFpsOnFocus, m_settings->fishstrapSupport);
                            }
                            ConfigManager::Save(*m_settings);

                            wchar_t notifyBuf[128];
                            if (m_settings->fpsLimit == 0) swprintf_s(notifyBuf, L(L"FPS Capper Disabled", L"Pengendali FPS Dinonaktifkan").c_str());
                            else swprintf_s(notifyBuf, L(L"FPS Limit set to %d FPS", L"Batas FPS disetel ke %d FPS").c_str(), m_settings->fpsLimit);
                            StatusBarOverlay::GetInstance().Show(notifyBuf, 1500, m_hwnd);
                        }
                    }
                    else if (rowIndex == 3) { // --- POPUP SELEKTOR BAHASA (NEW!) ---
                        HMENU hMenu = CreatePopupMenu();
                        AppendMenuW(hMenu, MF_STRING, 1, L"English");
                        AppendMenuW(hMenu, MF_STRING, 2, L"Indonesia");

                        POINT menuPt;
                        GetCursorPos(&menuPt);
                        int selection = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN, menuPt.x, menuPt.y, 0, hwnd, nullptr);
                        DestroyMenu(hMenu);

                        if (selection > 0) {
                            m_settings->language = selection - 1; // 0 = English, 1 = Indonesia
                            ConfigManager::Save(*m_settings);

                            // Update Watermark Placeholder dengan .c_str()
                            if (m_hEditWebhook) {
                                SendMessageW(m_hEditWebhook, EM_SETCUEBANNER, FALSE, (LPARAM)L(L"Type or paste Webhook URL here...", L"Ketik atau tempel URL Webhook di sini...").c_str());
                            }

                            StatusBarOverlay::GetInstance().Show(L(L"Language set to English", L"Bahasa disetel ke Indonesia"), 1500, m_hwnd);
                            
                            m_tabs[0].name = L(L"General", L"Umum");
                            m_tabs[1].name = L(L"Auto", L"Otomatis");
                            m_tabs[2].name = L(L"Stats", L"Statistik");
                            m_tabs[3].name = L(L"Advanced", L"Lanjutan");
                            m_tabs[4].name = L(L"Utils", L"Alat");
                            m_tabs[5].name = L(L"Webhook", L"Webhook");
                        }
                    }
                }
                else if (m_activeTab == 4) { // --- UTILS PAGE ---
                    if (rowIndex == 0) {
                        Core::RobloxDetector::GridSnapRobloxWindows(m_settings->fishstrapSupport);
                        StatusBarOverlay::GetInstance().Show(L(L"Windows Aligned to Grid", L"Jendela Diratakan ke Grid"), 1500, m_hwnd);
                    } else if (rowIndex == 1) {
                        Core::AntiAFKEngine::GetInstance().ForceTriggerAction(*m_settings);
                        StatusBarOverlay::GetInstance().Show(L(L"Test Action Dispatched", L"Tindakan Uji Dikirim"), 1500, m_hwnd);
                    } else if (rowIndex == 2) { // Impor Preset
                        if (ConfigManager::ImportPreset(hwnd, *m_settings)) {
                            if (m_hEditWebhook) SetWindowTextW(m_hEditWebhook, m_settings->discordWebhookUrl.c_str());
                            
                            m_tabs[0].name = L(L"General", L"Umum");
                            m_tabs[1].name = L(L"Auto", L"Otomatis");
                            m_tabs[2].name = L(L"Stats", L"Statistik");
                            m_tabs[3].name = L(L"Advanced", L"Lanjutan");
                            m_tabs[4].name = L(L"Utils", L"Alat");
                            m_tabs[5].name = L(L"Webhook", L"Webhook");

                            StatusBarOverlay::GetInstance().Show(L(L"Preset Imported Successfully!", L"Preset Berhasil Diimpor!"), 1800, m_hwnd);
                        } else {
                            StatusBarOverlay::GetInstance().Show(L(L"Import Cancelled", L"Impor Dibatalkan"), 1500, m_hwnd);
                        }
                    } else if (rowIndex == 3) { // Ekspor Preset
                        if (ConfigManager::ExportPreset(hwnd, *m_settings)) {
                            StatusBarOverlay::GetInstance().Show(L(L"Preset Exported Successfully!", L"Preset Berhasil Diekspor!"), 1800, m_hwnd);
                        } else {
                            StatusBarOverlay::GetInstance().Show(L(L"Export Cancelled", L"Ekspor Dibatalkan"), 1500, m_hwnd);
                        }
                    }
                }
                else if (m_activeTab == 5) { // --- WEBHOOK PAGE ---
                    if (rowIndex == 0) { 
                        m_settings->discordWebhookEnabled = !m_settings->discordWebhookEnabled;
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(m_settings->discordWebhookEnabled ? L(L"Discord Log: Enabled", L"Log Discord: AKTIF") : L(L"Discord Log: Disabled", L"Log Discord: MATI"), 1500, m_hwnd);
                    }
                    else if (rowIndex == 1) { 
                        if (m_hEditWebhook) SetFocus(m_hEditWebhook);
                    }
                    else if (rowIndex == 2) { 
                        m_settings->discordWebhookUrl = L"";
                        if (m_hEditWebhook) SetWindowTextW(m_hEditWebhook, L"");
                        ConfigManager::Save(*m_settings);
                        StatusBarOverlay::GetInstance().Show(L(L"Webhook URL Cleared", L"URL Webhook Bersih"), 1500, m_hwnd);
                    }
                    else if (rowIndex == 3) { 
                        if (m_settings->discordWebhookUrl.empty()) {
                            StatusBarOverlay::GetInstance().Show(L(L"Configure Webhook URL First!", L"Konfigurasi URL Webhook Dulu!"), 2000, m_hwnd);
                        } else {
                            StatusBarOverlay::GetInstance().Show(L(L"Sending Test Webhook...", L"Mengirim Tes Webhook..."), 1500, m_hwnd);
                            Network::DiscordWebhook::QueueEvent(Network::WebhookEvent::Test, *m_settings);
                        }
                    }
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        if (x >= 344 && x <= 380 && y <= 30) {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
    }

    void MainWindow::OnLButtonUp(HWND hwnd) {
        if (m_isPressingStart) {
            m_isPressingStart = false;
            
            auto& engine = Core::AntiAFKEngine::GetInstance();
            if (engine.IsRunning()) {
                uint64_t elapsed = engine.GetSessionElapsedSeconds();
                Network::DiscordWebhook::QueueEvent(Network::WebhookEvent::Stopped, *m_settings, elapsed);

                engine.Stop(*m_settings);
                StatusBarOverlay::GetInstance().Show(L(L"Engine Stopped", L"Mesin Dihentikan"), 1800, m_hwnd);
                
                ::UpdateTrayIcon(false, m_settings->multiSupport);
            } else {
                auto wins = Core::RobloxDetector::FindAllRobloxWindows(true, m_settings->fishstrapSupport);
                if (wins.empty()) {
                    StatusBarOverlay::GetInstance().Show(L(L"Warning: Roblox not found!", L"Peringatan: Roblox tidak ditemukan!"), 2000, m_hwnd);
                    Network::DiscordWebhook::QueueEvent(Network::WebhookEvent::Error, *m_settings);
                } else {
                    engine.Start(*m_settings, m_hwnd);
                    StatusBarOverlay::GetInstance().Show(L(L"Engine Started", L"Mesin Dijalankan"), 1800, m_hwnd);

                    Network::DiscordWebhook::QueueEvent(Network::WebhookEvent::Started, *m_settings);
                    
                    ::UpdateTrayIcon(true, m_settings->multiSupport);
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    void MainWindow::DrawToggleRow(Graphics& g, Font& font, int index, const std::wstring& icon, const std::wstring& label, bool checked, float animValue, float rowY) {
        Font iconFont(L"Segoe MDL2 Assets", 11.0f);
        SolidBrush textWhite(GetGdiColor(COLOR_TEXT_LIGHT));
        SolidBrush accentBrush(GetGdiColor(COLOR_ACCENT_PRIMARY));

        g.DrawString(icon.c_str(), -1, &iconFont, PointF(34.0f, rowY + 14.0f), &accentBrush);
        g.DrawString(label.c_str(), -1, &font, PointF(58.0f, rowY + 12.0f), &textWhite);

        REAL toggleW = 40.0f, toggleH = 20.0f;
        REAL toggleX = 380.0f - 16.0f - toggleW - 16.0f;
        REAL toggleY = rowY + 12.0f;

        COLORREF bgCol = AnimationHelper::LerpColor(COLOR_CARD_DARK, COLOR_ACCENT_PRIMARY, animValue);
        SolidBrush toggleBg(GetGdiColor(bgCol, 255));
        Pen toggleBorder(GetGdiColor(COLOR_TEXT_MUTED, 80), 1.2f);

        g.FillRectangle(&toggleBg, toggleX, toggleY, toggleW, toggleH);
        g.DrawRectangle(&toggleBorder, toggleX, toggleY, toggleW, toggleH);

        REAL knobSize = 14.0f;
        REAL knobX = toggleX + 3.0f + (toggleW - knobSize - 6.0f) * animValue;
        REAL knobY = toggleY + 3.0f;
        SolidBrush knobBrush(GetGdiColor(COLOR_TEXT_LIGHT));
        g.FillEllipse(&knobBrush, knobX, knobY, knobSize, knobSize);
    }

    void MainWindow::DrawSelectorRow(Graphics& g, Font& font, int index, const std::wstring& icon, const std::wstring& label, const std::wstring& value, float rowY) {
        Font iconFont(L"Segoe MDL2 Assets", 11.0f);
        SolidBrush textWhite(GetGdiColor(COLOR_TEXT_LIGHT));
        SolidBrush accentBrush(GetGdiColor(COLOR_ACCENT_PRIMARY));

        g.DrawString(icon.c_str(), -1, &iconFont, PointF(34.0f, rowY + 14.0f), &accentBrush);
        g.DrawString(label.c_str(), -1, &font, PointF(58.0f, rowY + 12.0f), &textWhite);

        Font valFont(L"Segoe UI", 8.5f, Gdiplus::FontStyleBold);
        REAL pillW = 90.0f, pillH = 22.0f;
        REAL pillX = 380.0f - 16.0f - pillW - 16.0f;
        REAL pillY = rowY + 11.0f;

        SolidBrush pillBg(GetGdiColor(COLOR_BG_DARK, 255));
        Pen pillBorder(GetGdiColor(COLOR_ACCENT_SECONDARY, 150), 1.0f);
        g.FillRectangle(&pillBg, pillX, pillY, pillW, pillH);
        g.DrawRectangle(&pillBorder, pillX, pillY, pillW, pillH);

        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        RectF textRect(pillX, pillY, pillW, pillH);
        g.DrawString(value.c_str(), -1, &valFont, textRect, &sf, &textWhite);
    }

    void MainWindow::OnPaint(HWND hwnd) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        SolidBrush bgBrush(GetGdiColor(COLOR_BG_DARK));
        g.FillRectangle(&bgBrush, 0, 0, w, h);

        Font headerFont(L"Segoe UI", 10.0f, Gdiplus::FontStyleBold);
        SolidBrush textWhite(GetGdiColor(COLOR_TEXT_LIGHT));
        g.DrawString(L"Anti-AFK HAB", -1, &headerFont, PointF(16.0f, 8.0f), &textWhite);

        Font iconFont(L"Segoe MDL2 Assets", 10.0f);
        SolidBrush xBrush(GetGdiColor(COLOR_TEXT_MUTED));
        g.DrawString(L"\uE8BB", -1, &iconFont, PointF(352.0f, 10.0f), &xBrush);

        float totalW = static_cast<float>(w);
        float activeTargetW = 110.0f; 
        float inactiveTargetW = (totalW - activeTargetW) / (m_tabs.size() - 1); 

        float currentX = 0.0f;
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            bool isActive = (m_activeTab == static_cast<int>(i));
            float targetW = isActive ? activeTargetW : inactiveTargetW;
            
            m_tabs[i].currentWidth = AnimationHelper::Lerp(m_tabs[i].currentWidth, targetW, 0.2f);
            m_tabs[i].bounds = { static_cast<int>(currentX), 35, static_cast<int>(currentX + m_tabs[i].currentWidth), 65 };

            SolidBrush tabBrush(GetGdiColor(isActive ? COLOR_TEXT_LIGHT : COLOR_TEXT_MUTED));
            Font tabIconFont(L"Segoe MDL2 Assets", 11.0f);

            if (isActive && m_tabs[i].currentWidth > 80.0f) {
                Font tabTextFont(L"Segoe UI", 8.5f, Gdiplus::FontStyleBold);
                g.DrawString(m_tabs[i].iconGlyph.c_str(), -1, &tabIconFont, PointF(currentX + 16.0f, 44.0f), &tabBrush);
                g.DrawString(m_tabs[i].name.c_str(), -1, &tabTextFont, PointF(currentX + 36.0f, 42.0f), &tabBrush);
            } else {
                g.DrawString(m_tabs[i].iconGlyph.c_str(), -1, &tabIconFont, PointF(currentX + (m_tabs[i].currentWidth - 11.0f) / 2.0f, 44.0f), &tabBrush);
            }
            currentX += m_tabs[i].currentWidth;
        }

        SolidBrush indicatorBrush(GetGdiColor(COLOR_ACCENT_PRIMARY));
        g.FillRectangle(&indicatorBrush, m_navIndicatorX + 8.0f, 62.0f, m_navIndicatorW - 16.0f, 3.0f);

        Pen dividerPen(GetGdiColor(COLOR_CARD_DARK, 180), 1.0f);
        g.DrawLine(&dividerPen, 0.0f, 65.0f, static_cast<REAL>(w), 65.0f);

        bool engineRunning = Core::AntiAFKEngine::GetInstance().IsRunning();

        SolidBrush cardBrush(GetGdiColor(COLOR_CARD_DARK));
        Pen cardPen(GetGdiColor(COLOR_TEXT_MUTED, 40), 1.0f);
        REAL panelX = 16.0f, panelY = 80.0f, panelW = w - 32.0f, panelH = h - 150.0f;
        GraphicsPath panelPath;
        REAL r = 8.0f;
        panelPath.AddArc(panelX, panelY, r * 2, r * 2, 180, 90);
        panelPath.AddArc(panelX + panelW - (r * 2), panelY, r * 2, r * 2, 270, 90);
        panelPath.AddArc(panelX + panelW - (r * 2), panelY + panelH - (r * 2), r * 2, r * 2, 0, 90);
        panelPath.AddArc(panelX, panelY + panelH - (r * 2), r * 2, r * 2, 90, 90);
        panelPath.CloseFigure();
        g.FillPath(&cardBrush, &panelPath);
        g.DrawPath(&cardPen, &panelPath);

        Font infoFont(L"Segoe UI", 9.0f);
        SolidBrush textMuted(GetGdiColor(COLOR_TEXT_MUTED));
        float startRowY = panelY + 12.0f;
        float rowHeight = 44.0f;

        if (m_activeTab == 0) { // --- GENERAL PAGE ---
            DrawToggleRow(g, infoFont, 0, L"\uE81E", L(L"Multi-Instance Support", L"Dukungan Multi-Instance"), m_settings->multiSupport, m_multiSupportAnim, startRowY);
            
            wchar_t intStr[64];
            swprintf_s(intStr, L"%d min", m_settings->selectedTime / 60);
            DrawSelectorRow(g, infoFont, 1, L"\uE916", L(L"Interval Timer", L"Waktu Jeda"), intStr, startRowY + rowHeight);

            const wchar_t* actionNames[] = { L"Space (Jump)", L"W/S Keys", L"Zoom I/O" };
            DrawSelectorRow(g, infoFont, 2, L"\uE7C9", L(L"AFK Action", L"Tindakan AFK"), actionNames[m_settings->selectedAction], startRowY + (rowHeight * 2));
        } 
        else if (m_activeTab == 1) { // --- AUTO PAGE ---
            DrawToggleRow(g, infoFont, 0, L"\uE768", L(L"Auto-Start AFK", L"Mulai AFK Otomatis"), m_settings->autoStartAfk, m_autoStartAnim, startRowY);
            DrawToggleRow(g, infoFont, 1, L"\uE8AF", L(L"Auto Reconnect", L"Penyambung Otomatis"), m_settings->autoReconnect, m_autoReconnectAnim, startRowY + rowHeight);
            DrawToggleRow(g, infoFont, 2, L"\uE81C", L(L"Auto Reset Player", L"Reset Player Otomatis"), m_settings->autoReset, m_autoResetAnim, startRowY + (rowHeight * 2));
            DrawToggleRow(g, infoFont, 3, L"\uEE47", L(L"Auto-Hide Roblox", L"Sembunyikan Roblox Otomatis"), m_settings->autoHideRoblox, m_autoHideAnim, startRowY + (rowHeight * 3));
        } 
        else if (m_activeTab == 2) { // --- STATS PAGE (LOKALISASI PRESEISI) ---
            // Ditambahkan .c_str() untuk memproses string lokalisasi
            g.DrawString(L(L"Session Statistics", L"Statistik Sesi").c_str(), -1, &headerFont, PointF(34.0f, startRowY + 4.0f), &textWhite);
            
            wchar_t statBuf[128];
            swprintf_s(statBuf, L"%s: %llu", L(L"Prevention Actions", L"Tindakan AFK").c_str(), m_settings->afkActions);
            g.DrawString(statBuf, -1, &infoFont, PointF(34.0f, startRowY + 36.0f), &textMuted);

            swprintf_s(statBuf, L"%s: %llu", L(L"Auto Reconnects", L"Penyambungan Ulang").c_str(), m_settings->autoReconnects);
            g.DrawString(statBuf, -1, &infoFont, PointF(34.0f, startRowY + 60.0f), &textMuted);

            uint64_t totalSeconds = m_settings->totalAfkTime + Core::AntiAFKEngine::GetInstance().GetSessionElapsedSeconds();
            if (m_settings->language == 1) {
                swprintf_s(statBuf, L"Total Waktu AFK: %llu jam %llu mnt %llu dtk", totalSeconds / 3600, (totalSeconds % 3600) / 60, totalSeconds % 60);
            } else {
                swprintf_s(statBuf, L"Total AFK Time: %llu hr %llu min %llu sec", totalSeconds / 3600, (totalSeconds % 3600) / 60, totalSeconds % 60);
            }
            g.DrawString(statBuf, -1, &infoFont, PointF(34.0f, startRowY + 84.0f), &textMuted);

            uint64_t currentSession = Core::AntiAFKEngine::GetInstance().GetSessionElapsedSeconds();
            uint64_t longestSec = (std::max)(m_settings->longestAfkSession, currentSession);
            if (m_settings->language == 1) {
                swprintf_s(statBuf, L"Sesi Terlama: %llu jam %llu mnt %llu dtk", longestSec / 3600, (longestSec % 3600) / 60, longestSec % 60);
            } else {
                swprintf_s(statBuf, L"Longest Session: %llu hr %llu min %llu sec", longestSec / 3600, (longestSec % 3600) / 60, longestSec % 60);
            }
            g.DrawString(statBuf, -1, &infoFont, PointF(34.0f, startRowY + 108.0f), &textMuted);

            DrawSelectorRow(g, infoFont, 3, L"\uE74D", L(L"Reset Statistics", L"Reset Statistik"), L(L"Reset Now", L"Reset Sekarang"), startRowY + (rowHeight * 3));
        } 
        else if (m_activeTab == 3) { // --- ADVANCED PAGE ---
            DrawToggleRow(g, infoFont, 0, L"\uF272", L(L"Unlock FPS on Focus", L"Buka Kunci FPS saat Fokus"), m_settings->unlockFpsOnFocus, m_unlockFpsAnim, startRowY);
            DrawToggleRow(g, infoFont, 1, L"\uE767", L(L"AutoMute Roblox", L"Bisukan Roblox Otomatis"), m_settings->autoMute, m_autoMuteAnim, startRowY + rowHeight);
            
            wchar_t fpsStr[64];
            if (m_settings->fpsLimit == 0) wcscpy_s(fpsStr, L"Off");
            else swprintf_s(fpsStr, L"%d FPS", m_settings->fpsLimit);
            DrawSelectorRow(g, infoFont, 2, L"\uF272", L(L"FPS Capper Limit", L"Batas Pengendali FPS"), fpsStr, startRowY + (rowHeight * 2));

            DrawSelectorRow(g, infoFont, 3, L"\uE7F6", L(L"App Language", L"Bahasa Aplikasi"), L(L"English", L"Indonesia"), startRowY + (rowHeight * 3));
        } 
        else if (m_activeTab == 4) { // --- UTILS PAGE ---
            DrawSelectorRow(g, infoFont, 0, L"\uE80A", L(L"Align Windows to Grid", L"Ratakan Jendela ke Grid"), L(L"Snap", L"Ratakan"), startRowY);
            DrawSelectorRow(g, infoFont, 1, L"\uE768", L(L"Force AFK Test", L"Uji Tindakan AFK"), L(L"Run", L"Uji"), startRowY + rowHeight);
            DrawSelectorRow(g, infoFont, 2, L"\uE896", L(L"Import Config Preset", L"Impor Preset Konfigurasi"), L(L"Import", L"Impor"), startRowY + (rowHeight * 2));
            DrawSelectorRow(g, infoFont, 3, L"\uE105", L(L"Export Config Preset", L"Ekspor Preset Konfigurasi"), L(L"Export", L"Ekspor"), startRowY + (rowHeight * 3));
        }
        else if (m_activeTab == 5) { // --- WEBHOOK PAGE ---
            DrawToggleRow(g, infoFont, 0, L"\uE8BD", L(L"Enable Discord Log", L"Aktifkan Log Discord"), m_settings->discordWebhookEnabled, m_settings->discordWebhookEnabled ? 1.0f : 0.0f, startRowY);
            
            Pen pillBorder(GetGdiColor(COLOR_ACCENT_SECONDARY, 150), 1.0f);
            g.DrawRectangle(&pillBorder, 32.0f, startRowY + rowHeight + 2.0f, 316.0f, 26.0f);

            DrawSelectorRow(g, infoFont, 2, L"\uE74D", L(L"Clear Webhook URL", L"Bersihkan URL Webhook"), L(L"Clear", L"Hapus"), startRowY + (rowHeight * 2));
            DrawSelectorRow(g, infoFont, 3, L"\uE104", L(L"Send Test Webhook", L"Kirim Tes Webhook"), L(L"Send Test", L"Kirim Tes"), startRowY + (rowHeight * 3));
        }

        int btnW = 140, btnH = 36;
        int btnX = (w - btnW) / 2;
        int btnY = h - 52;
        m_startBtnRect = { btnX, btnY, btnX + btnW, btnY + btnH };

        COLORREF btnColor = engineRunning ? COLOR_DANGER : COLOR_ACCENT_PRIMARY;
        SolidBrush actionBtnBrush(GetGdiColor(btnColor));

        g.ResetTransform();
        REAL pivotX = btnX + btnW / 2.0f;
        REAL pivotY = btnY + btnH / 2.0f;
        g.TranslateTransform(pivotX, pivotY);
        g.ScaleTransform(m_startBtnScale, m_startBtnScale);
        g.TranslateTransform(-pivotX, -pivotY);

        GraphicsPath btnPath;
        REAL btnRadius = 6.0f;
        btnPath.AddArc(static_cast<REAL>(btnX), static_cast<REAL>(btnY), btnRadius * 2, btnRadius * 2, 180, 90);
        btnPath.AddArc(static_cast<REAL>(btnX + btnW - (btnRadius * 2)), static_cast<REAL>(btnY), btnRadius * 2, btnRadius * 2, 270, 90);
        btnPath.AddArc(static_cast<REAL>(btnX + btnW - (btnRadius * 2)), static_cast<REAL>(btnY + btnH - (btnRadius * 2)), btnRadius * 2, btnRadius * 2, 0, 90);
        btnPath.AddArc(static_cast<REAL>(btnX), static_cast<REAL>(btnY + btnH - (btnRadius * 2)), btnRadius * 2, btnRadius * 2, 90, 90);
        btnPath.CloseFigure();

        g.FillPath(&actionBtnBrush, &btnPath);

        std::wstring actionText = engineRunning 
    ? L(L"Stop Engine", L"Hentikan Mesin") 
    : L(L"Start Engine", L"Mulai Mesin");

        Font btnFont(L"Segoe UI", 9.5f, Gdiplus::FontStyleBold);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        RectF textRect(static_cast<REAL>(btnX), static_cast<REAL>(btnY), static_cast<REAL>(btnW), static_cast<REAL>(btnH));
        g.DrawString(actionText.c_str(), -1, &btnFont, textRect, &sf, &textWhite);

        g.ResetTransform();

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
    }
}