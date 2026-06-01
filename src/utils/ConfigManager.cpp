#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <shlwapi.h>
#include <commdlg.h> 

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comdlg32.lib") 

namespace HAB {
    std::map<std::string, std::wstring> ConfigManager::m_links;

    void ConfigManager::LoadLinks() {
        m_links = {
            {"discord", L"https://discord.gg/jcfFAvKWvF"},
            {"github", L"https://github.com/habNoir/NotAFK-HAB"},
            {"wiki", L"https://discord.gg/jcfFAvKWvF"},
            {"sourceforge", L"https://discord.gg/jcfFAvKWvF"},
            {"tips", L"https://discord.gg/jcfFAvKWvF"}
        };

        std::ifstream file("links.json");
        if (!file.is_open()) return;

        std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len > 0) {
            std::wstring wstr(len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);
            
            std::wstring val;
            if (ParseJsonString(wstr, L"discord", val)) m_links["discord"] = val;
            if (ParseJsonString(wstr, L"github", val)) m_links["github"] = val;
            if (ParseJsonString(wstr, L"wiki", val)) m_links["wiki"] = val;
            if (ParseJsonString(wstr, L"sourceforge", val)) m_links["sourceforge"] = val;
            if (ParseJsonString(wstr, L"tips", val)) m_links["tips"] = val;
        }
    }

    std::wstring ConfigManager::GetLink(const std::string& key) {
        if (m_links.empty()) LoadLinks();
        auto it = m_links.find(key);
        return (it != m_links.end()) ? it->second : L"";
    }

    bool ConfigManager::ParseJsonString(const std::wstring& json, const std::wstring& key, std::wstring& value) {
        size_t pos = json.find(L"\"" + key + L"\"");
        if (pos == std::wstring::npos) return false;
        pos = json.find(L':', pos);
        if (pos == std::wstring::npos) return false;
        pos = json.find(L'"', pos);
        if (pos == std::wstring::npos) return false;
        size_t end = json.find(L'"', pos + 1);
        if (end == std::wstring::npos) return false;
        value = json.substr(pos + 1, end - pos - 1);
        return true;
    }

    bool ConfigManager::ParseJsonBool(const std::wstring& json, const std::wstring& key, bool& val) {
        size_t pos = json.find(L"\"" + key + L"\"");
        if (pos == std::wstring::npos) return false;
        pos = json.find(L':', pos);
        if (pos == std::wstring::npos) return false;
        size_t nextComma = json.find(L',', pos);
        size_t nextBrace = json.find(L'}', pos);
        size_t end = (std::min)(nextComma, nextBrace);
        std::wstring sub = json.substr(pos + 1, end - pos - 1);
        sub.erase(0, sub.find_first_not_of(L" \t\r\n"));
        sub.erase(sub.find_last_not_of(L" \t\r\n") + 1);
        val = (sub == L"true" || sub == L"1");
        return true;
    }

    bool ConfigManager::ParseJsonInt(const std::wstring& json, const std::wstring& key, int& val) {
        size_t pos = json.find(L"\"" + key + L"\"");
        if (pos == std::wstring::npos) return false;
        pos = json.find(L':', pos);
        if (pos == std::wstring::npos) return false;
        size_t nextComma = json.find(L',', pos);
        size_t nextBrace = json.find(L'}', pos);
        size_t end = (std::min)(nextComma, nextBrace);
        std::wstring sub = json.substr(pos + 1, end - pos - 1);
        sub.erase(0, sub.find_first_not_of(L" \t\r\n"));
        sub.erase(sub.find_last_not_of(L" \t\r\n") + 1);
        try {
            val = std::stoi(sub);
            return true;
        } catch (...) {
            return false;
        }
    }

    void ConfigManager::Load(Settings& s) {
        s = Settings();
    }

    void ConfigManager::Save(const Settings& s) {
        // Zero persistence otomatis dinonaktifkan demi kebersihan OS
    }

    void ConfigManager::Reset(Settings& s) {
        s = Settings();
    }

    bool ConfigManager::ExportPreset(HWND hwndOwner, const Settings& s) {
        wchar_t filename[MAX_PATH] = L"preset.json";
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwndOwner;
        ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = L"json";

        if (GetSaveFileNameW(&ofn)) {
            std::wstringstream ss;
            ss << L"{\n"
               << L"  \"language\": " << s.language << L",\n" // Simpan bahasa
               << L"  \"multiSupport\": " << (s.multiSupport ? L"true" : L"false") << L",\n"
               << L"  \"fishstrapSupport\": " << (s.fishstrapSupport ? L"true" : L"false") << L",\n"
               << L"  \"selectedTime\": " << s.selectedTime << L",\n"
               << L"  \"selectedAction\": " << s.selectedAction << L",\n"
               << L"  \"userSafeMode\": " << s.userSafeMode << L",\n"
               << L"  \"autoStartAfk\": " << (s.autoStartAfk ? L"true" : L"false") << L",\n"
               << L"  \"autoReconnect\": " << (s.autoReconnect ? L"true" : L"false") << L",\n"
               << L"  \"autoReset\": " << (s.autoReset ? L"true" : L"false") << L",\n"
               << L"  \"autoHideRoblox\": " << (s.autoHideRoblox ? L"true" : L"false") << L",\n"
               << L"  \"restoreMethod\": " << s.restoreMethod << L",\n"
               << L"  \"fpsLimit\": " << s.fpsLimit << L",\n"
               << L"  \"unlockFpsOnFocus\": " << (s.unlockFpsOnFocus ? L"true" : L"false") << L",\n"
               << L"  \"multiInstanceInterval\": " << s.multiInstanceInterval << L",\n"
               << L"  \"autoMute\": " << (s.autoMute ? L"true" : L"false") << L",\n"
               << L"  \"discordWebhookEnabled\": " << (s.discordWebhookEnabled ? L"true" : L"false") << L",\n"
               << L"  \"discordWebhookUrl\": \"" << s.discordWebhookUrl << L"\"\n"
               << L"}";

            std::wstring jsonStr = ss.str();
            std::string utf8Str;
            int len = WideCharToMultiByte(CP_UTF8, 0, jsonStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (len > 0) {
                utf8Str.resize(len - 1);
                WideCharToMultiByte(CP_UTF8, 0, jsonStr.c_str(), -1, &utf8Str[0], len, nullptr, nullptr);
            }

            std::ofstream out(filename, std::ios::binary);
            if (out.is_open()) {
                out.write(utf8Str.data(), utf8Str.size());
                out.close();
                return true;
            }
        }
        return false;
    }

    bool ConfigManager::ImportPreset(HWND hwndOwner, Settings& s) {
        wchar_t filename[MAX_PATH] = L"";
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwndOwner;
        ofn.lpstrFilter = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

        if (GetOpenFileNameW(&ofn)) {
            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) return false;

            std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
            if (len > 0) {
                std::wstring wstr(len, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);

                ParseJsonInt(wstr, L"language", s.language); // Muat bahasa
                ParseJsonBool(wstr, L"multiSupport", s.multiSupport);
                ParseJsonBool(wstr, L"fishstrapSupport", s.fishstrapSupport);
                ParseJsonInt(wstr, L"selectedTime", s.selectedTime);
                ParseJsonInt(wstr, L"selectedAction", s.selectedAction);
                ParseJsonInt(wstr, L"userSafeMode", s.userSafeMode);
                ParseJsonBool(wstr, L"autoStartAfk", s.autoStartAfk);
                ParseJsonBool(wstr, L"autoReconnect", s.autoReconnect);
                ParseJsonBool(wstr, L"autoReset", s.autoReset);
                ParseJsonBool(wstr, L"autoHideRoblox", s.autoHideRoblox);
                ParseJsonInt(wstr, L"restoreMethod", s.restoreMethod);
                ParseJsonInt(wstr, L"fpsLimit", s.fpsLimit);
                ParseJsonBool(wstr, L"unlockFpsOnFocus", s.unlockFpsOnFocus);
                ParseJsonInt(wstr, L"multiInstanceInterval", s.multiInstanceInterval);
                ParseJsonBool(wstr, L"autoMute", s.autoMute);
                ParseJsonBool(wstr, L"discordWebhookEnabled", s.discordWebhookEnabled);
                ParseJsonString(wstr, L"discordWebhookUrl", s.discordWebhookUrl);

                return true;
            }
        }
        return false;
    }
}