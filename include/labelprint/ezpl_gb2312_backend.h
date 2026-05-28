#pragma once

#include "labelprint/backend.h"

namespace labelprint {

// Godex EZPL backend with native Simplified Chinese via Z1/Z2 and CP936 bytes.
class EzplGb2312Backend : public IPrinterBackend {
public:
    PrintJob render(const LabelDocument& doc,
                    const PrinterProfile& profile) override;
};

} // namespace labelprint
