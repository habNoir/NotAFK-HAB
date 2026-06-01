#pragma once
#include <windows.h>
#include <gdiplus.h>

namespace HAB::UI {
    // HAB Premium Obsidian / Indigo Theme
    constexpr COLORREF COLOR_BG_DARK        = RGB(12, 12, 16);     // Deep Obsidian Base
    constexpr COLORREF COLOR_CARD_DARK      = RGB(22, 22, 30);     // Carbon Gray panel
    constexpr COLORREF COLOR_ACCENT_PRIMARY = RGB(99, 102, 241);   // Digital Indigo Glow
    constexpr COLORREF COLOR_ACCENT_SECONDARY= RGB(6, 182, 212);   // Cyber Teal glow
    constexpr COLORREF COLOR_TEXT_MUTED     = RGB(148, 163, 184);  // Slate Gray
    constexpr COLORREF COLOR_TEXT_LIGHT     = RGB(248, 250, 252);  // Porcelain White
    constexpr COLORREF COLOR_DANGER         = RGB(239, 68, 68);    // Vivid Coral Red

    inline Gdiplus::Color GetGdiColor(COLORREF c, BYTE alpha = 255) {
        return Gdiplus::Color(alpha, GetRValue(c), GetGValue(c), GetBValue(c));
    }
}