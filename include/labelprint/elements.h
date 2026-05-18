#pragma once

#include <string>

namespace labelprint {

// ---------------------------------------------------------------------------
// Text rendering strategy
// ---------------------------------------------------------------------------
enum class TextRenderMode {
    Native,  // Use printer built-in font
    Bitmap,  // Rasterize to bitmap before sending
    Auto     // Let backend decide based on content and printer capabilities
};

// ---------------------------------------------------------------------------
// Logical barcode type (mapped to printer commands by each backend)
// ---------------------------------------------------------------------------
enum class BarcodeType {
    Code128,
    Code39,
    EAN13,
    QRCode
};

// ---------------------------------------------------------------------------
// Logical font names (mapped to printer-specific fonts by each backend)
// ---------------------------------------------------------------------------
namespace Font {
    inline constexpr const char* Default = "default";
    inline constexpr const char* Small   = "small";
    inline constexpr const char* Medium  = "medium";
    inline constexpr const char* Large   = "large";
    inline constexpr const char* Bold    = "bold";
}

// ---------------------------------------------------------------------------
// TextElement — a single line of text
// ---------------------------------------------------------------------------
struct TextElement {
    int x = 0;
    int y = 0;
    std::string text;
    int height = 30;    // character height in dots
    int width  = 20;    // character width in dots
    std::string fontName = Font::Default;
    TextRenderMode renderMode = TextRenderMode::Auto;
};

// ---------------------------------------------------------------------------
// BarcodeElement — machine-readable barcode
// ---------------------------------------------------------------------------
struct BarcodeElement {
    int x = 0;
    int y = 0;
    std::string data;
    BarcodeType type = BarcodeType::Code128;
    int height = 100;        // bar height in dots
    int narrowWidth = 2;     // narrow bar width in dots
    double wideRatio = 3.0;  // wide:narrow ratio
    bool printTextBelow = true;
};

// ---------------------------------------------------------------------------
// LineElement — horizontal or vertical line
// ---------------------------------------------------------------------------
struct LineElement {
    int x = 0;
    int y = 0;
    int length = 0;
    bool horizontal = true;
    int thickness = 1;
};

// ---------------------------------------------------------------------------
// BoxElement — outlined rectangle
// ---------------------------------------------------------------------------
struct BoxElement {
    int x = 0;
    int y = 0;
    int width  = 0;
    int height = 0;
    int thickness = 1;
};

} // namespace labelprint
