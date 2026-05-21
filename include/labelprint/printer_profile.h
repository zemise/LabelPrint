#pragma once

#include <string>

namespace labelprint {

// ---------------------------------------------------------------------------
// Printer language
// ---------------------------------------------------------------------------
enum class PrinterLanguage {
    ZPL,
    TSPL
};

// ---------------------------------------------------------------------------
// Text rendering strategy — per-printer recommendation
// ---------------------------------------------------------------------------
enum class TextStrategy {
    Native,  // Use printer built-in font
    Bitmap,  // Rasterize to bitmap (required for CJK on many printers)
    Auto     // Native for ASCII, bitmap for CJK
};

// ---------------------------------------------------------------------------
// PrinterProfile — capabilities and defaults for a specific printer model
//
// This is pure data. It describes what a printer can do and what its
// preferred settings are. It does NOT contain rendering logic.
// ---------------------------------------------------------------------------
struct PrinterProfile {
    std::string name;
    PrinterLanguage language = PrinterLanguage::TSPL;
    int dpi = 203;

    // Maximum printable area in dots
    int maxWidth  = 812;
    int maxHeight = 1200;

    // --- Capability flags ---
    bool nativeBarcode  = true;
    bool nativeChinese  = false;   // built-in CJK font support
    bool bitmapGraphics = true;    // BITMAP / ~DG command support

    // Recommended text strategy for this printer
    TextStrategy textStrategy = TextStrategy::Auto;
    std::string nativeChineseFont; // e.g. "E:CSONG.TTF" for ZPL ^A@
    std::string nativeChineseCodepage; // e.g. "54936" for TSPL GB18030

    // --- Physical defaults ---
    int gapMm       = 2;    // label gap
    int gapOffsetMm = 0;    // gap offset
    int darkness    = 12;   // printer-native darkness (0-15 TSPL, 0-30 ZPL)
    int speed       = 4;    // inches per second
    int homeX       = 0;    // origin offset X in dots
    int homeY       = 0;    // origin offset Y in dots

    // --- BITMAP polarity (empirically determined per printer) ---
    // true  = white background bits → 1, black text bits → 0
    // false = black text bits → 1, white background bits → 0
    bool bitmapWhiteIsOne = false;
};

// ---------------------------------------------------------------------------
// Built-in printer profiles
// ---------------------------------------------------------------------------
namespace PrinterProfiles {

    // Xprinter XP-360B (203 DPI, TSPL) — verified working path
    const PrinterProfile& xprinter_xp360b();

    // Zebra ZD888 (203 DPI, ZPL) — reference profile
    const PrinterProfile& zebra_zd888();

} // namespace PrinterProfiles

} // namespace labelprint
