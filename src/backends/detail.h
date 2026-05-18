#pragma once

#include "labelprint/document.h"
#include "labelprint/printer_profile.h"
#include "zpl_label.h"
#include <vector>

namespace labelprint {
namespace detail {

// Map logical font name → ZPL font char
inline const char* mapFont(const std::string& fontName) {
    if (fontName == Font::Small)  return zpl::FONT_SMALL;
    if (fontName == Font::Medium) return zpl::FONT_MEDIUM;
    if (fontName == Font::Large)  return zpl::FONT_LARGE;
    if (fontName == Font::Bold)   return zpl::FONT_BOLD;
    return zpl::FONT_DEFAULT;
}

// Convert labelprint::LabelSettings → zpl::LabelSettings
inline zpl::LabelSettings convertSettings(const LabelSettings& s, const PrinterProfile& p) {
    zpl::LabelSettings out;
    out.labelWidth  = s.width;
    out.labelHeight = s.height;
    out.dpi         = p.dpi;
    out.homeX       = s.homeX;
    out.homeY       = s.homeY;
    out.darkness    = s.darkness;
    out.printSpeed  = s.printSpeed;
    out.quantity    = s.quantity;
    out.tearOff     = s.tearOff;
    return out;
}

// Populate a ZplLabel from a LabelDocument
inline void populate(zpl::ZplLabel& label, const LabelDocument& doc, const PrinterProfile& profile) {
    label.setSettings(convertSettings(doc.settings(), profile));

    for (const auto& t : doc.texts()) {
        label.text(t.x, t.y, t.text, t.height, t.width, mapFont(t.fontName));
    }

    for (const auto& b : doc.barcodes()) {
        label.barcode(b.x, b.y, b.data, b.height, b.narrowWidth, b.printTextBelow);
    }

    for (const auto& l : doc.lines()) {
        label.addLine(l.x, l.y, l.length, l.horizontal, l.thickness);
    }

    for (const auto& bx : doc.boxes()) {
        label.addBox(bx.x, bx.y, bx.width, bx.height, bx.thickness);
    }
}

// Populate a ZplLabel with per-element text strategy.
// Text elements needing bitmap rendering are collected in 'bitmaps'.
// Implemented in tspl_bitmap_backend.cpp (requires GDI+).
void populateTsplBitmap(zpl::ZplLabel& label,
                         std::vector<zpl::BitmapField>& bitmaps,
                         const LabelDocument& doc,
                         const PrinterProfile& profile);

} // namespace detail
} // namespace labelprint
