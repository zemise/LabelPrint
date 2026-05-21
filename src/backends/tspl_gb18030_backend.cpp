#include "labelprint/tspl_gb18030_backend.h"
#include "labelprint/elements.h"
#include "detail.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <iconv.h>
#endif

namespace labelprint {

namespace {

int clampInt(int value, int lo, int hi) {
    return std::max(lo, std::min(value, hi));
}

int dotsToMm(int dots, int dpi) {
    return static_cast<int>(dots * 25.4 / dpi + 0.5);
}

int dotsToTsplScale(int dots, int dpi) {
    int step = dpi * 24 / 203;
    return clampInt((dots + step - 1) / step, 1, 10);
}

bool containsNonAscii(const std::string& text) {
    for (unsigned char c : text) {
        if (c > 0x7F) {
            return true;
        }
    }
    return false;
}

std::string sanitizeTsplText(const std::string& input) {
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

#ifdef _WIN32
std::string utf8ToGb18030(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                      static_cast<int>(utf8.size()),
                                      nullptr, 0);
    if (wideLen <= 0) {
        throw std::runtime_error("UTF-8 to UTF-16 conversion failed");
    }

    std::wstring wide(static_cast<size_t>(wideLen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                        &wide[0], wideLen);

    int bytesLen = WideCharToMultiByte(54936, 0, wide.data(), wideLen,
                                       nullptr, 0, nullptr, nullptr);
    if (bytesLen <= 0) {
        bytesLen = WideCharToMultiByte(936, 0, wide.data(), wideLen,
                                       nullptr, 0, nullptr, nullptr);
        if (bytesLen <= 0) {
            throw std::runtime_error("UTF-16 to GB18030 conversion failed");
        }
        std::string out(static_cast<size_t>(bytesLen), '\0');
        WideCharToMultiByte(936, 0, wide.data(), wideLen, &out[0], bytesLen,
                            nullptr, nullptr);
        return out;
    }

    std::string out(static_cast<size_t>(bytesLen), '\0');
    WideCharToMultiByte(54936, 0, wide.data(), wideLen, &out[0], bytesLen,
                        nullptr, nullptr);
    return out;
}
#else
std::string utf8ToGb18030(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }

    iconv_t cd = iconv_open("GB18030", "UTF-8");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        throw std::runtime_error("iconv_open GB18030 failed");
    }

    std::string input = utf8;
    size_t inLeft = input.size();
    char* inPtr = input.data();

    std::string out(input.size() * 4 + 16, '\0');
    size_t outLeft = out.size();
    char* outPtr = &out[0];

    while (true) {
        size_t rc = iconv(cd, &inPtr, &inLeft, &outPtr, &outLeft);
        if (rc != static_cast<size_t>(-1)) {
            break;
        }
        if (errno != E2BIG) {
            iconv_close(cd);
            throw std::runtime_error("UTF-8 to GB18030 conversion failed");
        }
        size_t used = static_cast<size_t>(outPtr - out.data());
        out.resize(out.size() * 2);
        outPtr = &out[0] + used;
        outLeft = out.size() - used;
    }

    out.resize(static_cast<size_t>(outPtr - out.data()));
    iconv_close(cd);
    return out;
}
#endif

void put(std::vector<uint8_t>& out, const std::string& s) {
    out.insert(out.end(), s.begin(), s.end());
}

void putLine(std::vector<uint8_t>& out, const std::string& s) {
    put(out, s);
    out.push_back('\r');
    out.push_back('\n');
}

void putTextLine(std::vector<uint8_t>& out,
                 int x,
                 int y,
                 const std::string& font,
                 int xmul,
                 int ymul,
                 const std::string& text) {
    put(out, "TEXT " + std::to_string(x) + "," + std::to_string(y)
             + ",\"" + font + "\",0," + std::to_string(xmul) + ","
             + std::to_string(ymul) + ",\"");
    put(out, utf8ToGb18030(sanitizeTsplText(text)));
    putLine(out, "\"");
}

} // namespace

PrintJob TsplGb18030Backend::render(const LabelDocument& doc,
                                    const PrinterProfile& profile) {
    zpl::LabelSettings settings = detail::convertSettings(doc.settings(), profile);
    std::vector<uint8_t> out;
    std::ostringstream debug;

    int density = clampInt(settings.darkness / 2, 0, 15);
    const std::string codepage = profile.nativeChineseCodepage.empty()
                                     ? "54936"
                                     : profile.nativeChineseCodepage;
    const std::string chineseFont = profile.nativeChineseFont.empty()
                                        ? "TSS24.BF2"
                                        : profile.nativeChineseFont;

    auto line = [&](const std::string& s) {
        putLine(out, s);
        debug << s << "\r\n";
    };

    line("SIZE " + std::to_string(dotsToMm(settings.labelWidth, settings.dpi))
         + " mm," + std::to_string(dotsToMm(settings.labelHeight, settings.dpi)) + " mm");
    line("GAP 2 mm,0 mm");
    line("DENSITY " + std::to_string(density));
    line("SPEED " + std::to_string(settings.printSpeed));
    line("DIRECTION 1");
    line("REFERENCE " + std::to_string(settings.homeX) + ","
         + std::to_string(settings.homeY));
    line("CODEPAGE " + codepage);
    line("CLS");

    for (const auto& l : doc.lines()) {
        if (l.horizontal) {
            line("BAR " + std::to_string(l.x) + "," + std::to_string(l.y)
                 + "," + std::to_string(l.length) + "," + std::to_string(l.thickness));
        } else {
            line("BAR " + std::to_string(l.x) + "," + std::to_string(l.y)
                 + "," + std::to_string(l.thickness) + "," + std::to_string(l.length));
        }
    }

    for (const auto& box : doc.boxes()) {
        line("BOX " + std::to_string(box.x) + "," + std::to_string(box.y)
             + "," + std::to_string(box.x + box.width) + ","
             + std::to_string(box.y + box.height) + ","
             + std::to_string(box.thickness));
    }

    for (const auto& t : doc.texts()) {
        int xmul = dotsToTsplScale(t.width, settings.dpi);
        int ymul = dotsToTsplScale(t.height, settings.dpi);
        const std::string font = containsNonAscii(t.text) ? chineseFont : "3";
        putTextLine(out, t.x, t.y, font, xmul, ymul, t.text);
        debug << "TEXT " << t.x << "," << t.y << ",\"" << font << "\",0,"
              << xmul << "," << ymul << ",\"" << sanitizeTsplText(t.text)
              << "\"\r\n";
    }

    for (const auto& b : doc.barcodes()) {
        if (!b.data.empty()) {
            int narrow = clampInt(b.narrowWidth, 1, 10);
            int wide = clampInt(static_cast<int>(b.narrowWidth * b.wideRatio),
                                narrow + 1, 10);
            line("BARCODE " + std::to_string(b.x) + "," + std::to_string(b.y)
                 + ",\"128\"," + std::to_string(b.height) + ","
                 + (b.printTextBelow ? "1" : "0") + ",0,"
                 + std::to_string(narrow) + "," + std::to_string(wide)
                 + ",\"" + sanitizeTsplText(b.data) + "\"");
        }
    }

    line("PRINT " + std::to_string(clampInt(settings.quantity, 1, 9999)) + ",1");

    PrintJob job;
    job.format = "tspl-gb18030";
    job.data = std::move(out);
    job.debugText = debug.str();
    return job;
}

} // namespace labelprint
