#include "labelprint/tspl_bitmap_backend.h"
#include "detail.h"
#include "zpl_label.h"

#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <algorithm>

namespace labelprint {

namespace {

// ---------------------------------------------------------------------------
// CJK / non-ASCII detection
// In UTF-8, any byte > 0x7F belongs to a multi-byte sequence (CJK etc.).
// TSPL built-in fonts only reliably render ASCII.
// ---------------------------------------------------------------------------
bool containsNonAscii(const std::string& text) {
    for (unsigned char c : text) {
        if (c > 0x7F) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Per-element text rendering strategy
// Priority: element.renderMode → profile.textStrategy → auto-detect
// ---------------------------------------------------------------------------
bool shouldUseBitmap(const TextElement& elem, const PrinterProfile& profile) {
    if (elem.renderMode == TextRenderMode::Bitmap) return true;
    if (elem.renderMode == TextRenderMode::Native) return false;
    if (profile.textStrategy == TextStrategy::Bitmap) return true;
    if (profile.textStrategy == TextStrategy::Native) return false;
    return containsNonAscii(elem.text);
}

// ---------------------------------------------------------------------------
// UTF-8 → UTF-16 conversion (for GDI+)
// ---------------------------------------------------------------------------
std::wstring toWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &out[0], len);
    out.resize(len - 1); // remove null terminator
    return out;
}

// ---------------------------------------------------------------------------
// GDI+ one-time init
// ---------------------------------------------------------------------------
ULONG_PTR gdiplusToken() {
    static ULONG_PTR token = [] {
        Gdiplus::GdiplusStartupInput input;
        ULONG_PTR t = 0;
        Gdiplus::GdiplusStartup(&t, &input, nullptr);
        return t;
    }();
    return token;
}

// ---------------------------------------------------------------------------
// Rasterize text to a 1-bit-per-pixel bitmap suitable for TSPL BITMAP command.
// Returns packed bytes: MSB = leftmost pixel, white background bits = 1.
// ---------------------------------------------------------------------------
struct BitmapResult {
    int width;
    int height;
    std::vector<uint8_t> bytes;
};

BitmapResult renderTextToBitmap(const std::string& text,
                                 int targetWidth, int targetHeight,
                                 const std::string& fontName,
                                 float fontSize, bool whiteIsOne) {
    (void)gdiplusToken(); // ensure GDI+ is initialized

    std::wstring wtext = toWide(text);
    if (wtext.empty()) {
        // No text to render: return a blank (all-white) minimal bitmap
        int wb = (targetWidth + 7) / 8;
        BitmapResult blank;
        blank.width  = targetWidth;
        blank.height = targetHeight;
        blank.bytes.assign(wb * targetHeight, whiteIsOne ? 0xFF : 0x00);
        return blank;
    }

    // Create device-independent bitmap
    Gdiplus::Bitmap bitmap(targetWidth, targetHeight, PixelFormat32bppRGB);
    Gdiplus::Graphics graphics(&bitmap);
    graphics.SetPageUnit(Gdiplus::UnitPixel);
    graphics.Clear(Gdiplus::Color::White);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);

    // Font fallback: try requested font, then common Chinese fonts
    const wchar_t* families[] = { L"SimHei", L"Microsoft YaHei", L"SimSun" };
    Gdiplus::Font* font = nullptr;
    int familyIdx = 0;

    // Try to match the requested font first (by converting to wide)
    std::wstring wFontName = toWide(fontName);
    for (const auto& f : families) {
        Gdiplus::Font* trial = new Gdiplus::Font(f, fontSize,
                                                  Gdiplus::FontStyleBold,
                                                  Gdiplus::UnitPixel);
        if (trial->GetLastStatus() == Gdiplus::Ok) {
            font = trial;
            break;
        }
        delete trial;
    }

    if (!font) {
        // Last resort: default font (won't render CJK well, but won't crash)
        font = new Gdiplus::Font(L"Arial", fontSize, Gdiplus::FontStyleBold,
                                  Gdiplus::UnitPixel);
    }

    Gdiplus::SolidBrush blackBrush(Gdiplus::Color::Black);
    Gdiplus::PointF origin(0.0f, 0.0f);
    graphics.DrawString(wtext.c_str(), -1, font, origin, &blackBrush);

    // Read pixels via LockBits
    Gdiplus::Rect rect(0, 0, targetWidth, targetHeight);
    Gdiplus::BitmapData bmpData;
    bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead,
                     PixelFormat32bppRGB, &bmpData);
    BYTE* scan0 = static_cast<BYTE*>(bmpData.Scan0);

    int widthBytes = (targetWidth + 7) / 8;
    std::vector<uint8_t> out(widthBytes * targetHeight, 0);
    uint8_t bgBit = whiteIsOne ? 1 : 0;

    for (int y = 0; y < targetHeight; ++y) {
        for (int xb = 0; xb < widthBytes; ++xb) {
            uint8_t byteVal = 0;
            for (int bit = 0; bit < 8; ++bit) {
                int x = xb * 8 + bit;
                if (x < targetWidth) {
                    BYTE* pixel = scan0 + y * bmpData.Stride + x * 4;
                    int avg = (pixel[2] + pixel[1] + pixel[0]) / 3; // B,G,R
                    bool isBlack = (avg < 180);
                    // whiteIsOne: white pixels → bit 1, black pixels → bit 0
                    if ((whiteIsOne && !isBlack) || (!whiteIsOne && isBlack)) {
                        byteVal |= (1 << (7 - bit));
                    }
                } else {
                    // Padding: fill with background color
                    if (bgBit) byteVal |= (1 << (7 - bit));
                }
            }
            out[y * widthBytes + xb] = byteVal;
        }
    }

    bitmap.UnlockBits(&bmpData);
    delete font;

    return {targetWidth, targetHeight, std::move(out)};
}

} // namespace

// ---------------------------------------------------------------------------
// populateTsplBitmap — shared helper for TsplBitmapBackend
// Populates ZplLabel with native elements; collects bitmaps separately.
// ---------------------------------------------------------------------------
void detail::populateTsplBitmap(zpl::ZplLabel& label,
                                 std::vector<zpl::BitmapField>& bitmaps,
                                 const LabelDocument& doc,
                                 const PrinterProfile& profile) {
    label.setSettings(detail::convertSettings(doc.settings(), profile));

    // Lines, boxes, barcodes → always native
    for (const auto& l : doc.lines()) {
        label.addLine(l.x, l.y, l.length, l.horizontal, l.thickness);
    }
    for (const auto& bx : doc.boxes()) {
        label.addBox(bx.x, bx.y, bx.width, bx.height, bx.thickness);
    }
    for (const auto& b : doc.barcodes()) {
        label.barcode(b.x, b.y, b.data, b.height, b.narrowWidth, b.printTextBelow);
    }

    // Text elements: per-element strategy
    for (const auto& t : doc.texts()) {
        if (shouldUseBitmap(t, profile)) {
            // Estimate bitmap size from element dimensions.
            // For Chinese text, characters are roughly square; width is apportioned
            // per estimated character count based on UTF-8 byte length.
            size_t charCount = 0;
            for (size_t i = 0; i < t.text.size(); ) {
                unsigned char c = static_cast<unsigned char>(t.text[i]);
                if (c <= 0x7F)       i += 1;
                else if (c <= 0xDF)  i += 2;
                else if (c <= 0xEF)  i += 3;
                else                 i += 4;
                ++charCount;
            }
            if (charCount == 0) charCount = 1;
            int bmpWidth  = std::max(8, static_cast<int>(t.width * static_cast<int>(charCount)));
            int bmpHeight = std::max(8, t.height);
            float fontSize = t.height * 0.85f;

            auto bmp = renderTextToBitmap(t.text, bmpWidth, bmpHeight,
                                           "SimHei", fontSize,
                                           profile.bitmapWhiteIsOne);

            zpl::BitmapField bf;
            bf.x      = t.x;
            bf.y      = t.y;
            bf.width  = bmp.width;
            bf.height = bmp.height;
            bf.data   = std::move(bmp.bytes);
            bitmaps.push_back(std::move(bf));
        } else {
            // ASCII: native TSPL TEXT command
            label.text(t.x, t.y, t.text, t.height, t.width,
                        detail::mapFont(t.fontName));
        }
    }
}

// ---------------------------------------------------------------------------
// TsplBitmapBackend::render
// ---------------------------------------------------------------------------
PrintJob TsplBitmapBackend::render(const LabelDocument& doc,
                                    const PrinterProfile& profile) {
    zpl::LabelSettings zplSettings = detail::convertSettings(doc.settings(), profile);
    zpl::ZplLabel label(zplSettings);
    std::vector<zpl::BitmapField> bitmaps;

    detail::populateTsplBitmap(label, bitmaps, doc, profile);

    for (auto& bm : bitmaps) {
        label.addBitmap(std::move(bm));
    }

    PrintJob job;
    job.format = "tspl-bitmap";
    job.data   = label.generateTsplBinary();

    // Human-readable debug text
    std::ostringstream debug;
    debug << label.generateTspl();
    for (const auto& bm : label.bitmaps()) {
        debug << "\n// (BITMAP " << bm.width << "x" << bm.height
              << " at " << bm.x << "," << bm.y << ")";
    }
    job.debugText = debug.str();

    return job;
}

} // namespace labelprint
