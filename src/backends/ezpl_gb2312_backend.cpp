#include "labelprint/ezpl_gb2312_backend.h"
#include "labelprint/elements.h"
#include "detail.h"

#include <algorithm>
#include <cerrno>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
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

bool containsNonAscii(const std::string& text) {
    for (unsigned char c : text) {
        if (c > 0x7F) {
            return true;
        }
    }
    return false;
}

std::string sanitizeEzplText(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        if (ch == '\r' || ch == '\n' || ch == ',') {
            out += ' ';
        } else {
            out += ch;
        }
    }
    return out;
}

#ifdef _WIN32
std::string utf8ToCp936(const std::string& utf8) {
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

    int bytesLen = WideCharToMultiByte(936, 0, wide.data(), wideLen,
                                       nullptr, 0, nullptr, nullptr);
    if (bytesLen <= 0) {
        throw std::runtime_error("UTF-16 to CP936 conversion failed");
    }

    std::string out(static_cast<size_t>(bytesLen), '\0');
    WideCharToMultiByte(936, 0, wide.data(), wideLen, &out[0], bytesLen,
                        nullptr, nullptr);
    return out;
}
#else
std::string utf8ToCp936(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }

    iconv_t cd = iconv_open("GBK", "UTF-8");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        cd = iconv_open("CP936", "UTF-8");
        if (cd == reinterpret_cast<iconv_t>(-1)) {
            throw std::runtime_error("iconv_open CP936 failed");
        }
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
            throw std::runtime_error("UTF-8 to CP936 conversion failed");
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

std::string ezplAsciiFont(const TextElement& text) {
    if (text.height <= 18) {
        return "Z1";
    }
    if (text.fontName == Font::Small) {
        return "B";
    }
    if (text.fontName == Font::Large || text.fontName == Font::Bold) {
        return "D";
    }
    return "C";
}

std::string ezplChineseFont(const TextElement& text, const PrinterProfile& profile) {
    if (!profile.nativeChineseFont.empty()) {
        return profile.nativeChineseFont;
    }
    return text.height <= 18 ? "Z1" : "Z2";
}

int textScale(int requestedDots, int baseDots) {
    return clampInt((requestedDots + baseDots - 1) / baseDots, 1, 8);
}

void putTextLine(std::vector<uint8_t>& out,
                 std::ostringstream& debug,
                 const TextElement& text,
                 const PrinterProfile& profile,
                 int offsetX,
                 int offsetY) {
    const std::string sanitized = sanitizeEzplText(text.text);
    const bool chinese = containsNonAscii(sanitized);
    const std::string font = chinese ? ezplChineseFont(text, profile) : ezplAsciiFont(text);
    const int base = chinese && font == "Z1" ? 16 : 24;
    const int xmul = textScale(text.width, base);
    const int ymul = textScale(text.height, base);

    const int x = text.x + offsetX;
    const int y = text.y + offsetY;

    put(out, "A" + font + "," + std::to_string(x) + ","
             + std::to_string(y) + "," + std::to_string(xmul) + ","
             + std::to_string(ymul) + ",0,0,");
    if (chinese) {
        put(out, utf8ToCp936(sanitized));
    } else {
        put(out, sanitized);
    }
    putLine(out, "");

    debug << "A" << font << "," << x << "," << y << ","
          << xmul << "," << ymul << ",0,0," << sanitized << "\r\n";
}

} // namespace

PrintJob EzplGb2312Backend::render(const LabelDocument& doc,
                                   const PrinterProfile& profile) {
    zpl::LabelSettings settings = detail::convertSettings(doc.settings(), profile);
    std::vector<uint8_t> out;
    std::ostringstream debug;

    auto line = [&](const std::string& s) {
        putLine(out, s);
        debug << s << "\r\n";
    };

    line("~S,ESG");
    line("^Q" + std::to_string(dotsToMm(settings.labelHeight, settings.dpi)) + ",2");
    line("^W" + std::to_string(dotsToMm(settings.labelWidth, settings.dpi)));
    line("^H" + std::to_string(clampInt(settings.darkness / 2, 0, 19)));
    line("^S" + std::to_string(clampInt(settings.printSpeed, 1, 7)));
    line("^P" + std::to_string(clampInt(settings.quantity, 1, 9999)));
    line("^L");

    for (const auto& t : doc.texts()) {
        putTextLine(out, debug, t, profile, settings.homeX, settings.homeY);
    }

    for (const auto& b : doc.barcodes()) {
        if (!b.data.empty()) {
            int narrow = clampInt(b.narrowWidth, 1, 10);
            int wide = clampInt(static_cast<int>(b.narrowWidth * b.wideRatio),
                                narrow + 1, 10);
            line("BQ," + std::to_string(b.x + settings.homeX) + ","
                 + std::to_string(b.y + settings.homeY)
                 + "," + std::to_string(narrow) + "," + std::to_string(wide)
                 + "," + std::to_string(b.height) + ",0,"
                 + (b.printTextBelow ? "1" : "0") + ","
                 + sanitizeEzplText(b.data));
        }
    }

    line("E");

    PrintJob job;
    job.format = "ezpl-gb2312";
    job.data = std::move(out);
    job.debugText = debug.str();
    return job;
}

} // namespace labelprint
