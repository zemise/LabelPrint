#include "labelprint/printer_profile.h"

namespace labelprint {

const PrinterProfile& PrinterProfiles::xprinter_xp360b() {
    static const PrinterProfile p = [] {
        PrinterProfile p;
        p.name             = "Xprinter XP-360B";
        p.language         = PrinterLanguage::TSPL;
        p.dpi              = 203;
        p.maxWidth         = 812;
        p.maxHeight        = 1200;
        p.nativeBarcode    = true;
        p.nativeChinese    = true;
        p.bitmapGraphics   = true;
        p.textStrategy     = TextStrategy::Auto;
        p.nativeChineseFont = "TSS24.BF2";
        p.nativeChineseCodepage = "54936";
        p.gapMm            = 2;
        p.gapOffsetMm      = 0;
        p.darkness         = 25;   // ZPL range (0-30); maps to TSPL DENSITY=12
        p.speed            = 4;
        p.homeX            = 0;
        p.homeY            = 0;
        p.bitmapWhiteIsOne = true;   // verified: white bg bits = 1
        return p;
    }();
    return p;
}

const PrinterProfile& PrinterProfiles::zebra_zd888() {
    static const PrinterProfile p = [] {
        PrinterProfile p;
        p.name             = "Zebra ZD888";
        p.language         = PrinterLanguage::ZPL;
        p.dpi              = 203;
        p.maxWidth         = 812;
        p.maxHeight        = 1200;
        p.nativeBarcode    = true;
        p.nativeChinese    = true;    // verified with E:CSONG.TTF on the device
        p.bitmapGraphics   = true;
        p.textStrategy     = TextStrategy::Native;
        p.nativeChineseFont = "E:CSONG.TTF";
        p.gapMm            = 2;
        p.gapOffsetMm      = 0;
        p.darkness         = 25;
        p.speed            = 4;
        p.homeX            = 0;
        p.homeY            = 0;
        p.bitmapWhiteIsOne = false;
        return p;
    }();
    return p;
}

const PrinterProfile& PrinterProfiles::godex_g500u() {
    static const PrinterProfile p = [] {
        PrinterProfile p;
        p.name             = "Godex G500U";
        p.language         = PrinterLanguage::ZPL; // GZPL-compatible command path
        p.dpi              = 203;
        p.maxWidth         = 864;   // 108 mm * 8 dots/mm
        p.maxHeight        = 13816; // 1727 mm * 8 dots/mm
        p.nativeBarcode    = true;
        p.nativeChinese    = false; // Brochure lists Asian fonts, but no verified GZPL font name.
        p.bitmapGraphics   = true;
        p.textStrategy     = TextStrategy::Auto;
        p.gapMm            = 2;
        p.gapOffsetMm      = 0;
        p.darkness         = 25;
        p.speed            = 5;
        p.homeX            = 0;
        p.homeY            = 0;
        p.bitmapWhiteIsOne = false;
        return p;
    }();
    return p;
}

} // namespace labelprint
