#include "AudioManager.h"
#include <wrl/client.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <tlhelp32.h>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace HAB::Core {
    bool AudioManager::MuteProcess(DWORD pid, bool mute) {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bool success = false;

        ComPtr<IMMDeviceEnumerator> enumerator;
        if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) {
            ComPtr<IMMDevice> device;
            if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
                ComPtr<IAudioSessionManager2> sessionManager;
                if (SUCCEEDED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, &sessionManager))) {
                    ComPtr<IAudioSessionEnumerator> sessionEnumerator;
                    if (SUCCEEDED(sessionManager->GetSessionEnumerator(&sessionEnumerator))) {
                        int count = 0;
                        sessionEnumerator->GetCount(&count);
                        for (int i = 0; i < count; ++i) {
                            ComPtr<IAudioSessionControl> control;
                            if (SUCCEEDED(sessionEnumerator->GetSession(i, &control))) {
                                ComPtr<IAudioSessionControl2> control2;
                                if (SUCCEEDED(control.As(&control2))) {
                                    DWORD sessionPid = 0;
                                    if (SUCCEEDED(control2->GetProcessId(&sessionPid)) && sessionPid == pid) {
                                        ComPtr<ISimpleAudioVolume> volume;
                                        if (SUCCEEDED(control2.As(&volume))) {
                                            volume->SetMute(mute ? TRUE : FALSE, nullptr);
                                            success = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        CoUninitialize();
        return success;
    }

    bool AudioManager::IsProcessMuted(DWORD pid, bool& isMuted) {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        bool found = false;

        ComPtr<IMMDeviceEnumerator> enumerator;
        if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) {
            ComPtr<IMMDevice> device;
            if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
                ComPtr<IAudioSessionManager2> sessionManager;
                if (SUCCEEDED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, &sessionManager))) {
                    ComPtr<IAudioSessionEnumerator> sessionEnumerator;
                    if (SUCCEEDED(sessionManager->GetSessionEnumerator(&sessionEnumerator))) {
                        int count = 0;
                        sessionEnumerator->GetCount(&count);
                        for (int i = 0; i < count; ++i) {
                            ComPtr<IAudioSessionControl> control;
                            if (SUCCEEDED(sessionEnumerator->GetSession(i, &control))) {
                                ComPtr<IAudioSessionControl2> control2;
                                if (SUCCEEDED(control.As(&control2))) {
                                    DWORD sessionPid = 0;
                                    if (SUCCEEDED(control2->GetProcessId(&sessionPid)) && sessionPid == pid) {
                                        ComPtr<ISimpleAudioVolume> volume;
                                        if (SUCCEEDED(control2.As(&volume))) {
                                            BOOL muted = FALSE;
                                            if (SUCCEEDED(volume->GetMute(&muted))) {
                                                isMuted = (muted != FALSE);
                                                found = true;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        CoUninitialize();
        return found;
    }

    void AudioManager::MuteAllRoblox(bool mute, bool fishstrapSupport) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32W pe = { 0 };
        pe.dwSize = sizeof(pe);

        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"RobloxPlayerBeta.exe") == 0 || 
                    (fishstrapSupport && _wcsicmp(pe.szExeFile, L"eurotrucks2.exe") == 0)) {
                    MuteProcess(pe.th32ProcessID, mute);
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
}