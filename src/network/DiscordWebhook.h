#pragma once
#include <string>
#include "../utils/ConfigManager.h"

namespace HAB::Network {
    enum class WebhookEvent {
        Test,
        Started,
        Stopped,
        Action,
        Error
    };

    class DiscordWebhook {
    public:
        // Menerima parameter tambahan untuk menyusun isi Rich Embeds secara dinamis
        static void QueueEvent(WebhookEvent eventType, const Settings& settings, uint64_t elapsedSec = 0);
    
    private:
        static bool SendRequest(const std::wstring& webhookUrl, WebhookEvent eventType, const Settings& settings, uint64_t elapsedSec);
        static std::string BuildPayloadJson(WebhookEvent eventType, const Settings& settings, uint64_t elapsedSec);
        static std::string EscapeJsonString(const std::string& input);
        static std::string WideToUtf8(const std::wstring& str);
        static std::wstring Utf8ToWide(const std::string& str);
    };
}