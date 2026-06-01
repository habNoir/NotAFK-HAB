#pragma once
#include <windows.h>
#include <string>
#include <map>

namespace HAB {
    struct Settings {
        bool multiSupport = false;
        bool fishstrapSupport = false;
        int selectedTime = 540;
        int selectedAction = 0;
        bool autoUpdate = true;
        int userSafeMode = 0;
        bool autoStartAfk = false;
        bool autoReconnect = false;
        bool autoReset = false;
        bool autoHideRoblox = false;
        bool autoOpacity = false;
        bool autoGrid = false;
        int restoreMethod = 1;
        bool tutorialShown = false;
        uint64_t totalAfkTime = 0;
        uint64_t afkActions = 0;
        uint64_t autoReconnects = 0;
        uint64_t longestAfkSession = 0;
        uint64_t discordWebhooksSent = 0;
        uint64_t programLaunches = 0;
        uint64_t afkSessionsCompleted = 0;
        bool useLegacyUi = false;
        bool bloxstrapIntegration = false;
        bool statusBarEnabled = true;
        int fpsLimit = 0;
        bool unlockFpsOnFocus = false;
        int multiInstanceInterval = 0;
        bool windowOpacity = false;
        bool afkReminder = false;
        bool doNotSleep = false;
        bool autoMute = false;
        bool unmuteOnFocus = false;
        bool discordWebhookEnabled = false;
        bool discordNotifyStart = true;
        bool discordNotifyStop = true;
        bool discordNotifyAction = false;
        bool discordNotifyReconnect = true;
        bool discordNotifyReset = false;
        bool discordNotifyErrors = true;
        bool discordDisableEmbed = false;
        bool discordMentionOnErrors = false;
        std::wstring discordWebhookUrl = L"";
        
        // REVISI: Tambahkan pilihan bahasa (0 = English, 1 = Indonesia)
        int language = 1; 
    };

    class ConfigManager {
    public:
        static void Load(Settings& settings);
        static void Save(const Settings& settings);
        static void Reset(Settings& settings);
        static std::wstring GetLink(const std::string& key);
        
        static bool ExportPreset(HWND hwndOwner, const Settings& settings);
        static bool ImportPreset(HWND hwndOwner, Settings& settings);
        static bool ParseJsonString(const std::wstring& json, const std::wstring& key, std::wstring& value);

    private:
        static std::map<std::string, std::wstring> m_links;
        static void LoadLinks();
        static bool ParseJsonBool(const std::wstring& json, const std::wstring& key, bool& val);
        static bool ParseJsonInt(const std::wstring& json, const std::wstring& key, int& val);
    };
}