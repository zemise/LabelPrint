#include "zpl_label.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>

namespace zpl {

namespace {

std::string escapeTspl(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        if (ch == '"') {
            out += '\'';
        } else if (ch == '\r' || ch == '\n') {
            out += ' ';
        } else {
            out += ch;
        }
    }
    return out;
}

int clampInt(int value, int lo, int hi) {
    return std::max(lo, std::min(value, hi));
}

int dotsToTsplScale(int dots, int dpi) {
    int step = dpi * 24 / 203;   // TSPL font step: 24 dots at 203 DPI
    return clampInt((dots + step - 1) / step, 1, 10);
}

int dotsToMm(int dots, int dpi) {
    return static_cast<int>(dots * 25.4 / dpi + 0.5);
}

} // namespace

// ---------------------------------------------------------------------------
// Add elements (return *this for chaining)
// ---------------------------------------------------------------------------
ZplLabel& ZplLabel::addText(const TextField& t) {
    texts_.push_back(t);
    return *this;
}

ZplLabel& ZplLabel::addBarcode(const BarcodeField& b) {
    barcodes_.push_back(b);
    return *this;
}

ZplLabel& ZplLabel::addLine(int x, int y, int length, bool horizontal, int thickness) {
    GraphicsField g;
    g.x = x; g.y = y;
    if (horizontal) { g.width = length; g.height = 0; }
    else            { g.width = 0;     g.height = length; }
    g.thickness = thickness;
    g.type = GraphicsField::Line;
    graphics_.push_back(g);
    return *this;
}

ZplLabel& ZplLabel::addBox(int x, int y, int w, int h, int thickness) {
    GraphicsField g;
    g.x = x; g.y = y; g.width = w; g.height = h; g.thickness = thickness;
    g.type = GraphicsField::Box;
    graphics_.push_back(g);
    return *this;
}

ZplLabel& ZplLabel::addBitmap(int x, int y, int w, int h,
                               const std::vector<uint8_t>& data) {
    BitmapField bf;
    bf.x = x; bf.y = y; bf.width = w; bf.height = h; bf.data = data;
    bitmaps_.push_back(std::move(bf));
    return *this;
}

ZplLabel& ZplLabel::addBitmap(BitmapField&& bf) {
    bitmaps_.push_back(std::move(bf));
    return *this;
}

// ---------------------------------------------------------------------------
// Quick helpers
// ---------------------------------------------------------------------------
ZplLabel& ZplLabel::text(int x, int y, const std::string& str,
                          int h, int w, const std::string& font) {
    return addText({x, y, str, font, h, w, Orient::Normal});
}

ZplLabel& ZplLabel::textTtf(int x, int y, const std::string& str,
                              int h, int w, const std::string& fontPath) {
    TextField tf{x, y, str, "", h, w, Orient::Normal, true, fontPath};
    return addText(tf);
}

ZplLabel& ZplLabel::barcode(int x, int y, const std::string& data,
                             int barHeight, int modW, bool printText) {
    return addBarcode({x, y, data, barHeight, modW, 3.0, printText, Orient::Normal, "N"});
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------
void ZplLabel::clear() {
    texts_.clear();
    barcodes_.clear();
    graphics_.clear();
    bitmaps_.clear();
}

// ---------------------------------------------------------------------------
// Helper: ZPL header
// ---------------------------------------------------------------------------
std::string ZplLabel::head() const {
    std::ostringstream z;
    z << "^XA";                                           // start
    z << "^CI28";                                         // UTF-8 encoding (required for CJK)
    z << "^LH" << settings_.homeX << "," << settings_.homeY; // home
    z << "^PW" << settings_.labelWidth;                    // print width
    z << "^LL" << settings_.labelHeight;                   // label length
    z << "~SD" << settings_.darkness;                      // darkness
    z << "^PR" << settings_.printSpeed;                    // print speed
    if (settings_.tearOff) z << "^MMT";                    // tear-off
    z << "^PQ" << settings_.quantity;                      // quantity
    return z.str();
}

// ---------------------------------------------------------------------------
// Helper: ZPL footer
// ---------------------------------------------------------------------------
std::string ZplLabel::foot() const {
    return "^XZ";
}

// ---------------------------------------------------------------------------
// Generate full ZPL string
// ---------------------------------------------------------------------------
std::string ZplLabel::generate() const {
    std::ostringstream z;
    z << head();

    // --- Graphics first (bottom layer) ---
    for (const auto& g : graphics_) {
        z << "^FO" << g.x << "," << g.y
          << "^GB" << g.width << "," << g.height << "," << g.thickness << "^FS";
    }

    // --- Text fields ---
    for (const auto& t : texts_) {
        z << "^FO" << t.x << "," << t.y;
        if (t.useTtf) {
            // TrueType font: ^A@o,h,w,drive:filename.ext
            z << "^A@" << static_cast<char>(t.orient)
              << "," << t.height << "," << t.width
              << "," << t.ttfName;
        } else {
            // Built-in bitmap font: ^Afo,h,w
            z << "^A" << t.font << static_cast<char>(t.orient)
              << "," << t.height << "," << t.width;
        }
        z << "^FD" << t.text << "^FS";
    }

    // --- Barcode fields (top layer) ---
    for (const auto& b : barcodes_) {
        if (!b.data.empty()) {
            z << "^FO" << b.x << "," << b.y;
            // ^BY: module width, ratio, height  — put height in ^BC for per-barcode control
            z << "^BY" << b.moduleWidth << "," << b.ratio << "," << b.height;
            z << "^BC" << static_cast<char>(b.orient) << "," << b.height
              << "," << (b.printText ? 'Y' : 'N') << ",N,N," << b.mode;
            z << "^FD" << b.data << "^FS";
        }
    }

    z << foot();
    return z.str();
}

// ---------------------------------------------------------------------------
// Save to file
// ---------------------------------------------------------------------------
void ZplLabel::save(const std::string& filepath) const {
    std::ofstream f(filepath);
    if (!f) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    f << generate();
}

// ---------------------------------------------------------------------------
// Generate TSPL string for Xprinter-style label printers
// ---------------------------------------------------------------------------
std::string ZplLabel::generateTspl() const {
    std::ostringstream z;
    int density = clampInt(settings_.darkness / 2, 0, 15);

    z << "SIZE " << dotsToMm(settings_.labelWidth, settings_.dpi) << " mm," << dotsToMm(settings_.labelHeight, settings_.dpi) << " mm\r\n";
    z << "GAP 2 mm,0 mm\r\n";
    z << "DENSITY " << density << "\r\n";
    z << "SPEED " << settings_.printSpeed << "\r\n";
    z << "DIRECTION 1\r\n";
    z << "REFERENCE " << settings_.homeX << "," << settings_.homeY << "\r\n";
    z << "CLS\r\n";

    for (const auto& g : graphics_) {
        if (g.type == GraphicsField::Box) {
            z << "BOX " << g.x << "," << g.y << "," << (g.x + g.width) << "," << (g.y + g.height)
              << "," << g.thickness << "\r\n";
        } else if (g.width > 0) {
            z << "BAR " << g.x << "," << g.y << "," << g.width << "," << g.thickness << "\r\n";
        } else if (g.height > 0) {
            z << "BAR " << g.x << "," << g.y << "," << g.thickness << "," << g.height << "\r\n";
        }
    }

    for (const auto& t : texts_) {
        int xmul = dotsToTsplScale(t.width,  settings_.dpi);
        int ymul = dotsToTsplScale(t.height, settings_.dpi);
        z << "TEXT " << t.x << "," << t.y << ",\"3\",0," << xmul << "," << ymul
          << ",\"" << escapeTspl(t.text) << "\"\r\n";
    }

    for (const auto& b : barcodes_) {
        if (!b.data.empty()) {
            int narrow = clampInt(b.moduleWidth, 1, 10);
            int wide = clampInt(static_cast<int>(b.moduleWidth * b.ratio), narrow + 1, 10);
            z << "BARCODE " << b.x << "," << b.y << ",\"128\"," << b.height << ","
              << (b.printText ? 1 : 0) << ",0," << narrow << "," << wide
              << ",\"" << escapeTspl(b.data) << "\"\r\n";
        }
    }

    z << "PRINT " << clampInt(settings_.quantity, 1, 9999) << ",1\r\n";
    return z.str();
}

void ZplLabel::saveTspl(const std::string& filepath) const {
    std::ofstream f(filepath, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    f << generateTspl();
}

// ---------------------------------------------------------------------------
// Preview: send ZPL to Labelary API, get PNG back
// ---------------------------------------------------------------------------
bool ZplLabel::preview(const std::string& outputPng, int dpi, int scale) const {
    // Convert DPI to dpmm for Labelary URL
    int dpmm;
    switch (dpi) {
        case 300: dpmm = 12; break;
        case 600: dpmm = 24; break;
        default:  dpmm = 8;  break;  // 203 DPI
    }

    double wIn = settings_.labelWidth  / static_cast<double>(dpi);
    double hIn = settings_.labelHeight / static_cast<double>(dpi);

    // Write ZPL to a temp file
    std::string tmpZpl = outputPng + ".tmp.zpl";
    try {
        save(tmpZpl);
    } catch (...) {
        return false;
    }

    // Step 1: Download 1x PNG from Labelary API
    std::string rawPng = outputPng + ".raw.png";
    char url[256];
    std::snprintf(url, sizeof(url),
        "http://api.labelary.com/v1/printers/%ddpmm/labels/%.2fx%.2f/0/",
        dpmm, wIn, hIn);

    std::string curlCmd = "curl -sS -o '" + rawPng + "' "
                          "-H 'Accept: image/png' "
                          "--data-binary '@" + tmpZpl + "' "
                          "'" + url + "' 2>&1";
    int rc = std::system(curlCmd.c_str());
    std::remove(tmpZpl.c_str());
    if (rc != 0) {
        std::remove(rawPng.c_str());
        return false;
    }

    // Step 2: Upscale by `scale` factor using nearest-neighbor for sharp edges.
    // Try Python PIL first (NEAREST = pixel-perfect), fall back to sips (macOS).
    if (scale > 1) {
        int newW = settings_.labelWidth  * scale;
        int newH = settings_.labelHeight * scale;

        // Python PIL: nearest-neighbor preserves hard barcode edges
        char pyCmd[512];
        std::snprintf(pyCmd, sizeof(pyCmd),
            "python3 -c \""
            "from PIL import Image; "
            "img=Image.open('%s'); "
            "img=img.resize((%d,%d), Image.NEAREST); "
            "img.save('%s')\" 2>/dev/null",
            rawPng.c_str(), newW, newH, outputPng.c_str());

        rc = std::system(pyCmd);
        if (rc != 0) {
            // Fallback: macOS sips (built-in)
            char sipsCmd[512];
            std::snprintf(sipsCmd, sizeof(sipsCmd),
                "sips -z %d %d '%s' --out '%s' 2>/dev/null",
                newH, newW, rawPng.c_str(), outputPng.c_str());
            rc = std::system(sipsCmd);
        }

        std::remove(rawPng.c_str());
        return rc == 0;
    }

    // scale == 1: just rename
    std::rename(rawPng.c_str(), outputPng.c_str());
    return true;
}
// ---------------------------------------------------------------------------
// Generate TSPL as binary vector (supports embedded BITMAP data)
// ---------------------------------------------------------------------------
std::vector<uint8_t> ZplLabel::generateTsplBinary() const {
    std::vector<uint8_t> out;

    auto put = [&](const std::string& s) {
        out.insert(out.end(), s.begin(), s.end());
    };
    auto putLine = [&](const std::string& s) {
        out.insert(out.end(), s.begin(), s.end());
        out.push_back('\r');
        out.push_back('\n');
    };

    int density = clampInt(settings_.darkness / 2, 0, 15);

    putLine("SIZE " + std::to_string(dotsToMm(settings_.labelWidth, settings_.dpi))
            + " mm," + std::to_string(dotsToMm(settings_.labelHeight, settings_.dpi)) + " mm");
    putLine("GAP 2 mm,0 mm");
    putLine("DENSITY " + std::to_string(density));
    putLine("SPEED " + std::to_string(settings_.printSpeed));
    putLine("DIRECTION 1");
    putLine("REFERENCE " + std::to_string(settings_.homeX) + ","
            + std::to_string(settings_.homeY));
    putLine("CLS");

    // Graphics
    for (const auto& g : graphics_) {
        if (g.type == GraphicsField::Box) {
            putLine("BOX " + std::to_string(g.x) + "," + std::to_string(g.y)
                    + "," + std::to_string(g.x + g.width) + ","
                    + std::to_string(g.y + g.height) + ","
                    + std::to_string(g.thickness));
        } else if (g.width > 0) {
            putLine("BAR " + std::to_string(g.x) + "," + std::to_string(g.y)
                    + "," + std::to_string(g.width) + ","
                    + std::to_string(g.thickness));
        } else if (g.height > 0) {
            putLine("BAR " + std::to_string(g.x) + "," + std::to_string(g.y)
                    + "," + std::to_string(g.thickness) + ","
                    + std::to_string(g.height));
        }
    }

    // Native text elements
    for (const auto& t : texts_) {
        int xmul = dotsToTsplScale(t.width,  settings_.dpi);
        int ymul = dotsToTsplScale(t.height, settings_.dpi);
        putLine("TEXT " + std::to_string(t.x) + "," + std::to_string(t.y)
                + ",\"3\",0," + std::to_string(xmul) + "," + std::to_string(ymul)
                + ",\"" + escapeTspl(t.text) + "\"");
    }

    // Barcode elements
    for (const auto& b : barcodes_) {
        if (!b.data.empty()) {
            int narrow = clampInt(b.moduleWidth, 1, 10);
            int wide = clampInt(static_cast<int>(b.moduleWidth * b.ratio), narrow + 1, 10);
            putLine("BARCODE " + std::to_string(b.x) + "," + std::to_string(b.y)
                    + ",\"128\"," + std::to_string(b.height) + ","
                    + (b.printText ? "1" : "0") + ",0,"
                    + std::to_string(narrow) + "," + std::to_string(wide)
                    + ",\"" + escapeTspl(b.data) + "\"");
        }
    }

    // Bitmap elements (binary interleaved with ASCII header)
    for (const auto& bm : bitmaps_) {
        int widthBytes = (bm.width + 7) / 8;
        std::string header = "BITMAP " + std::to_string(bm.x) + ","
                           + std::to_string(bm.y) + ","
                           + std::to_string(widthBytes) + ","
                           + std::to_string(bm.height) + ",0,";
        put(header);
        out.insert(out.end(), bm.data.begin(), bm.data.end());
        out.push_back('\r');
        out.push_back('\n');
    }

    putLine("PRINT " + std::to_string(clampInt(settings_.quantity, 1, 9999)) + ",1");

    return out;
}

void ZplLabel::saveTsplBinary(const std::string& filepath) const {
    auto data = generateTsplBinary();
    std::ofstream f(filepath, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
}

} // namespace zpl

// ---------------------------------------------------------------------------
// openViewer – open Labelary viewer + copy ZPL to clipboard
// ---------------------------------------------------------------------------
void zpl::ZplLabel::openViewer() const {
    // 1) Write ZPL to temp file (avoid shell escaping / piping issues)
    std::string tmpZpl = "/tmp/zpl_viewer_input.zpl";
    save(tmpZpl);

    // 2) Copy ZPL to clipboard and open Labelary viewer
    //    User just needs Cmd+V to paste into the viewer
    std::system(
        "cat /tmp/zpl_viewer_input.zpl | pbcopy 2>/dev/null; "
        "open https://labelary.com/viewer.html 2>/dev/null || "
        "xdg-open https://labelary.com/viewer.html 2>/dev/null");

    std::fprintf(stderr, "[openViewer] ZPL copied to clipboard → Cmd+V in the viewer\n");
}
