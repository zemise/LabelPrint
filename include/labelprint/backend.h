#pragma once

#include "labelprint/document.h"
#include "labelprint/printer_profile.h"
#include <string>
#include <vector>
#include <cstdint>

namespace labelprint {

// ---------------------------------------------------------------------------
// PrintJob — backend render output
// ---------------------------------------------------------------------------
struct PrintJob {
    std::string format;             // "zpl", "tspl", "tspl-bitmap"
    std::vector<uint8_t> data;      // raw bytes ready for transport
    std::string debugText;          // human-readable form (optional)

    // Helpers for text-based formats
    std::string asText() const {
        return std::string(reinterpret_cast<const char*>(data.data()), data.size());
    }

    static PrintJob fromText(const std::string& format, const std::string& text) {
        PrintJob job;
        job.format = format;
        job.data.assign(text.begin(), text.end());
        job.debugText = text;
        return job;
    }
};

// ---------------------------------------------------------------------------
// IPrinterBackend — renders a LabelDocument into a PrintJob
// ---------------------------------------------------------------------------
class IPrinterBackend {
public:
    virtual ~IPrinterBackend() = default;
    virtual PrintJob render(const LabelDocument& doc,
                            const PrinterProfile& profile) = 0;
};

} // namespace labelprint
