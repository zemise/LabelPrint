#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdint>

namespace zpl {

// Font: use "0" (default), or single letter "A" through "H", or "0"-"9"
inline const char* FONT_DEFAULT = "0";
inline const char* FONT_SMALL   = "A";   // 9x5
inline const char* FONT_MEDIUM  = "C";   // 18x10
inline const char* FONT_LARGE   = "E";   // 28x15
inline const char* FONT_BOLD    = "G";   // 60x40

// Orientation
enum class Orient { Normal = 'N', Rotated = 'R', Inverted = 'I', BottomUp = 'B' };

// ---------------------------------------------------------------------------
// Label-level settings
// ---------------------------------------------------------------------------
struct LabelSettings {
    int  labelWidth  = 400;   // dots
    int  labelHeight = 600;   // dots
    int  dpi         = 203;   // dots per inch
    int  homeX       = 20;    // ^LH x offset
    int  homeY       = 20;    // ^LH y offset
    int  darkness    = 25;    // ~sd (0-30)
    int  printSpeed  = 4;     // ^PR (inches/sec)
    int  quantity    = 1;     // ^PQ
    bool tearOff     = true;  // ^MMT
};

// ---------------------------------------------------------------------------
// Text element
// ---------------------------------------------------------------------------
struct TextField {
    int  x, y;                    // position in dots
    std::string text;
    std::string font   = FONT_DEFAULT;
    int         height = 30;      // char height in dots
    int         width  = 20;      // char width in dots
    Orient      orient = Orient::Normal;
    bool        useTtf = false;   // true → ^A@ (TrueType), false → ^A (built-in)
    std::string ttfName;          // e.g. "E:SIMSUN.TTF" — font file on printer
};

// ---------------------------------------------------------------------------
// Barcode element (Code 128)
// ---------------------------------------------------------------------------
struct BarcodeField {
    int  x, y;
    std::string data;
    int    height      = 100;     // bar height in dots
    int    moduleWidth = 2;       // narrow bar width in dots (1-10)
    double ratio       = 3.0;     // wide:narrow ratio (2.0-3.0)
    bool   printText   = true;    // human-readable text below bars
    Orient orient      = Orient::Normal;
    std::string mode   = "N";     // N=normal, U=UCC case, A=auto, D=UCC/EAN
};

// ---------------------------------------------------------------------------
// Graphic: line or box
// ---------------------------------------------------------------------------
struct GraphicsField {
    int x, y;
    int width  = 0;   // 0 = vertical line, >0 = box or horizontal line
    int height = 0;   // >0 with width>0 = box; both 0 = ignored
    int thickness = 1;
    enum Type { Line, Box } type = Type::Line;
};

// ---------------------------------------------------------------------------
// Bitmap element (for TSPL BITMAP command)
// ---------------------------------------------------------------------------
struct BitmapField {
    int x = 0;
    int y = 0;
    int width = 0;                    // bitmap width in dots
    int height = 0;                   // bitmap height in dots
    std::vector<uint8_t> data;        // raw bytes: (widthBytes * height)
};

// ===========================================================================
// ZplLabel – assembles elements and emits ZPL
// ===========================================================================
class ZplLabel {
public:
    ZplLabel() = default;
    explicit ZplLabel(const LabelSettings& s) : settings_(s) {}

    // --- add elements ---
    ZplLabel& addText(const TextField& t);
    ZplLabel& addBarcode(const BarcodeField& b);
    ZplLabel& addLine(int x, int y, int length, bool horizontal = true, int thickness = 1);
    ZplLabel& addBox(int x, int y, int w, int h, int thickness = 1);

    // --- bitmap support (for TSPL BITMAP commands) ---
    ZplLabel& addBitmap(int x, int y, int w, int h,
                         const std::vector<uint8_t>& data);
    ZplLabel& addBitmap(BitmapField&& bf);
    const std::vector<BitmapField>& bitmaps() const { return bitmaps_; }

    // --- quick-add helpers (infer defaults) ---
    ZplLabel& text(int x, int y, const std::string& str,
                   int h = 30, int w = 20, const std::string& font = FONT_DEFAULT);
    // TTF text — for CJK / Unicode (^A@); fontPath e.g. "E:SIMSUN.TTF"
    ZplLabel& textTtf(int x, int y, const std::string& str,
                      int h, int w, const std::string& fontPath);
    ZplLabel& barcode(int x, int y, const std::string& data,
                      int barHeight = 100, int modW = 2, double ratio = 3.0,
                      bool printText = true);

    // --- settings ---
    void setSettings(const LabelSettings& s) { settings_ = s; }
    const LabelSettings& settings() const { return settings_; }

    // --- generate ---
    std::string generate() const;
    void save(const std::string& filepath) const;
    std::string generateTspl() const;
    void saveTspl(const std::string& filepath) const;
    std::vector<uint8_t> generateTsplBinary() const;
    void saveTsplBinary(const std::string& filepath) const;

    // --- preview: convert ZPL to PNG via Labelary API ---
    // dpi: 203 (default), 300, or 600
    // scale: integer multiplier for output image (default 3x for Retina screens)
    // returns true on success
    bool preview(const std::string& outputPng, int dpi = 203, int scale = 3) const;

    // --- open Labelary web viewer in browser with current ZPL ---
    void openViewer() const;

    // --- clear ---
    void clear();

private:
    std::string head() const;
    std::string foot() const;

    LabelSettings settings_;
    std::vector<TextField>    texts_;
    std::vector<BarcodeField> barcodes_;
    std::vector<GraphicsField> graphics_;
    std::vector<BitmapField>   bitmaps_;
};
} // namespace zpl
