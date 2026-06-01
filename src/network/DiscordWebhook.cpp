#include "DiscordWebhook.h"
#include <windows.h>
#include <wininet.h>
#include <thread>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Wininet.lib")

namespace HAB::Network {
    std::string DiscordWebhook::WideToUtf8(const std::wstring& str) {
        if (str.empty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &utf8[0], len, nullptr, nullptr);
        return utf8;
    }

    std::wstring DiscordWebhook::Utf8ToWide(const std::string& str) {
        if (str.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
        std::wstring wide(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &wide[0], len);
        return wide;
    }

    std::string DiscordWebhook::EscapeJsonString(const std::string& input) {
        std::string escaped;
        escaped.reserve(input.size() + 8);
        for (unsigned char ch : input) {
            switch (ch) {
                case '\"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (ch < 0x20) {
                        char buf[7];
                        sprintf_s(buf, "\\u%04X", ch);
                        escaped += buf;
                    } else {
                        escaped.push_back(ch);
                    }
                    break;
            }
        }
        return escaped;
    }

    std::string DiscordWebhook::BuildPayloadJson(WebhookEvent eventType, const Settings& settings, uint64_t elapsedSec) {
        std::string appVersion = "Anti-AFK HAB v1.0";
        std::ostringstream json;

        // Skema Warna Keren untuk Setiap Jenis Event
        DWORD embedColor = 0x6366F1; // Default Indigo
        std::string title = "";
        std::string desc = "";

        const wchar_t* actionNames[] = { L"Space (Jump)", L"W/S Keys", L"Zoom I/O" };
        std::string actionName = WideToUtf8(actionNames[settings.selectedAction]);

        switch (eventType) {
            case WebhookEvent::Test:
                embedColor = 0x06B6D4; // Cyber Teal
                title = "🧪 Webhook Connection Test";
                desc = "Koneksi ke Discord Webhook berhasil dikonfigurasi dengan aman!";
                break;

            case WebhookEvent::Started:
                embedColor = 0x6366F1; // Indigo
                title = "🚀 Anti-AFK Engine Started";
                {
                    std::ostringstream ss;
                    ss << "Siklus pencegahan AFK berhasil diaktifkan.\\n\\n"
                       << "**Konfigurasi Aktif:**\\n"
                       << "• Jeda Interval: `" << (settings.selectedTime / 60) << " menit`\\n"
                       << "• Metode Gerakan: `" << actionName << "`\\n"
                       << "• Multi-Instance: `" << (settings.multiSupport ? "Aktif" : "Non-Aktif") << "`";
                    desc = ss.str();
                }
                break;

            case WebhookEvent::Stopped:
                embedColor = 0xEF4444; // Coral Red
                title = "🛑 Anti-AFK Engine Stopped";
                {
                    uint64_t hr = elapsedSec / 3600;
                    uint64_t min = (elapsedSec % 3600) / 60;
                    uint64_t sec = elapsedSec % 60;

                    std::ostringstream ss;
                    ss << "Siklus pencegahan AFK telah dinonaktifkan.\\n\\n"
                       << "**Ringkasan Sesi Ini:**\\n"
                       << "• Durasi Aktif: `" << hr << " jam " << min << " menit " << sec << " detik`\\n"
                       << "• Total Aksi Terkirim: `" << settings.afkActions << " kali`\\n"
                       << "• Auto Reconnects: `" << settings.autoReconnects << " kali`";
                    desc = ss.str();
                }
                break;

            case WebhookEvent::Action:
                embedColor = 0x10B981; // Emerald Green
                title = "⚡ AFK Action Executed";
                {
                    std::ostringstream ss;
                    ss << "Simulasi input berhasil dikirim ke jendela game.\\n"
                       << "• Metode: `" << actionName << "`\\n"
                       << "• Jeda Berikutnya: `" << (settings.selectedTime / 60) << " menit`";
                    desc = ss.str();
                }
                break;

            case WebhookEvent::Error:
                embedColor = 0xF59E0B; // Amber Warning
                title = "⚠️ Warning: Roblox Not Found";
                desc = "Mesin anti-AFK aktif namun jendela game Roblox tidak ditemukan di layar monitor! Sistem sedang menunggu game dijalankan.";
                break;
        }

        title = EscapeJsonString(title);
        desc = EscapeJsonString(desc);

        if (settings.discordDisableEmbed) {
            std::string content = appVersion + " | " + title + " : " + desc;
            if (settings.discordMentionOnErrors && (eventType == WebhookEvent::Error)) {
                content = "@everyone " + content;
            }
            json << "{\"content\":\"" << content << "\"}";
        } else {
            std::string mentionBlock = "";
            if (settings.discordMentionOnErrors && (eventType == WebhookEvent::Error)) {
                mentionBlock = "\"content\":\"@everyone\",";
            }

            json << "{" << mentionBlock
                 << "\"embeds\":[{"
                 << "\"title\":\"" << title << "\","
                 << "\"description\":\"" << desc << "\","
                 << "\"color\":" << embedColor << ","
                 << "\"footer\":{\"text\":\"" << appVersion << "\"},"
                 << "\"timestamp\":\"\"" // Menyerahkan penulisan waktu otomatis ke server Discord
                 << "}]}";
        }
        return json.str();
    }

    bool DiscordWebhook::SendRequest(const std::wstring& webhookUrl, WebhookEvent eventType, const Settings& settings, uint64_t elapsedSec) {
        URL_COMPONENTSW comp = { 0 };
        comp.dwStructSize = sizeof(comp);
        wchar_t host[256] = { 0 };
        wchar_t path[2048] = { 0 };
        comp.lpszHostName = host;
        comp.dwHostNameLength = _countof(host);
        comp.lpszUrlPath = path;
        comp.dwUrlPathLength = _countof(path);

        if (!InternetCrackUrlW(webhookUrl.c_str(), 0, 0, &comp)) return false;

        HINTERNET hInternet = InternetOpenW(L"Anti-AFK HAB", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
        if (!hInternet) return false;

        bool success = false;
        HINTERNET hConnect = InternetConnectW(hInternet, host, comp.nPort, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
        if (hConnect) {
            const wchar_t* accept[] = { L"*/*", nullptr };
            HINTERNET hRequest = HttpOpenRequestW(hConnect, L"POST", path, nullptr, nullptr, accept, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
            if (hRequest) {
                std::string body = BuildPayloadJson(eventType, settings, elapsedSec);
                const wchar_t* headers = L"Content-Type: application/json\r\n";
                if (HttpSendRequestW(hRequest, headers, -1, (LPVOID)body.data(), static_cast<DWORD>(body.size()))) {
                    DWORD code = 0;
                    DWORD codeLen = sizeof(code);
                    if (HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &code, &codeLen, nullptr)) {
                        if (code >= 200 && code < 300) success = true;
                    }
                }
                InternetCloseHandle(hRequest);
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
        return success;
    }

    void DiscordWebhook::QueueEvent(WebhookEvent eventType, const Settings& settings, uint64_t elapsedSec) {
        if (eventType != WebhookEvent::Test && !settings.discordWebhookEnabled) return;

        std::wstring url = settings.discordWebhookUrl;
        if (url.empty()) return;

        std::thread([url, eventType, settings, elapsedSec]() {
            SendRequest(url, eventType, settings, elapsedSec);
        }).detach();
    }
}