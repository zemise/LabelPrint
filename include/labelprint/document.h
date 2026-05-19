#pragma once

#include "elements.h"
#include <vector>

namespace labelprint {

// ---------------------------------------------------------------------------
// LabelSettings — printer-independent label configuration
// ---------------------------------------------------------------------------
struct LabelSettings {
    int width      = 400;   // label width in dots
    int height     = 600;   // label height in dots
    int homeX      = 20;    // origin X offset in dots
    int homeY      = 20;    // origin Y offset in dots
    int darkness   = 25;    // print darkness (0-30)
    int printSpeed = 4;     // print speed (inches/sec)
    int quantity   = 1;     // number of copies
    bool tearOff   = true;  // tear-off mode
};

// ---------------------------------------------------------------------------
// LabelDocument — printer-independent label description
//
// This is the top-level model. It holds a collection of logical elements
// (text, barcodes, lines, boxes) with no knowledge of any printer language.
// Backends translate this document into ZPL, TSPL, or other formats.
// ---------------------------------------------------------------------------
class LabelDocument {
public:
    LabelDocument() = default;
    explicit LabelDocument(const LabelSettings& s) : settings_(s) {}

    // --- element accessors (read-only) ---
    const std::vector<TextElement>&    texts()    const { return texts_; }
    const std::vector<BarcodeElement>& barcodes() const { return barcodes_; }
    const std::vector<LineElement>&    lines()    const { return lines_; }
    const std::vector<BoxElement>&     boxes()    const { return boxes_; }

    // --- settings ---
    const LabelSettings& settings() const { return settings_; }
    void setSettings(const LabelSettings& s) { settings_ = s; }

    // --- add elements (returns reference to the stored element) ---
    TextElement& addText(const TextElement& t) {
        texts_.push_back(t);
        return texts_.back();
    }

    TextElement& addText(int x, int y, const std::string& text,
                         int h = 30, int w = 20,
                         const std::string& font = Font::Default) {
        return addText({x, y, text, h, w, font});
    }

    BarcodeElement& addBarcode(const BarcodeElement& b) {
        barcodes_.push_back(b);
        return barcodes_.back();
    }

    BarcodeElement& addBarcode(int x, int y, const std::string& data,
                               int barH = 100, int narrowW = 2,
                               double wideRatio = 3.0,
                               bool printText = true) {
        return addBarcode({x, y, data, BarcodeType::Code128,
                           barH, narrowW, wideRatio, printText});
    }

    LineElement& addLine(const LineElement& l) {
        lines_.push_back(l);
        return lines_.back();
    }

    LineElement& addLine(int x, int y, int length,
                         bool horizontal = true, int thickness = 1) {
        return addLine({x, y, length, horizontal, thickness});
    }

    BoxElement& addBox(const BoxElement& b) {
        boxes_.push_back(b);
        return boxes_.back();
    }

    BoxElement& addBox(int x, int y, int w, int h, int thickness = 1) {
        return addBox({x, y, w, h, thickness});
    }

    // --- clear all elements ---
    void clear() {
        texts_.clear();
        barcodes_.clear();
        lines_.clear();
        boxes_.clear();
    }

private:
    LabelSettings settings_;
    std::vector<TextElement>    texts_;
    std::vector<BarcodeElement> barcodes_;
    std::vector<LineElement>    lines_;
    std::vector<BoxElement>     boxes_;
};

} // namespace labelprint
