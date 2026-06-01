#pragma once
#include <cmath>
#include <windows.h>

namespace HAB::UI {
    class AnimationHelper {
    public:
        // Exponential interpolation (Smooth Lerp)
        static float Lerp(float current, float target, float speed = 0.15f) {
            if (std::abs(current - target) < 0.001f) {
                return target;
            }
            return current + (target - current) * speed;
        }

        // Color Lerp (Smooth transisi warna)
        static COLORREF LerpColor(COLORREF from, COLORREF to, float factor) {
            factor = (factor < 0.0f) ? 0.0f : ((factor > 1.0f) ? 1.0f : factor);
            BYTE r = static_cast<BYTE>(GetRValue(from) + factor * (GetRValue(to) - GetRValue(from)));
            BYTE g = static_cast<BYTE>(GetGValue(from) + factor * (GetGValue(to) - GetGValue(from)));
            BYTE b = static_cast<BYTE>(GetBValue(from) + factor * (GetBValue(to) - GetBValue(from)));
            return RGB(r, g, b);
        }
    };
}