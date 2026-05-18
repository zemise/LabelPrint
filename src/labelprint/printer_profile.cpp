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
        p.nativeChinese    = false;
        p.bitmapGraphics   = true;
        p.textStrategy     = TextStrategy::Auto;
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
        p.nativeChinese    = false;   // needs TTF font file on printer
        p.bitmapGraphics   = true;
        p.textStrategy     = TextStrategy::Auto;
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

} // namespace labelprint
