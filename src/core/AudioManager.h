#pragma once
#include <windows.h>

namespace HAB::Core {
    class AudioManager {
    public:
        static bool MuteProcess(DWORD pid, bool mute);
        static bool IsProcessMuted(DWORD pid, bool& isMuted);
        static void MuteAllRoblox(bool mute, bool fishstrapSupport);
    };
}