#pragma once

#include "labelprint/backend.h"

namespace labelprint {

class TsplBackend : public IPrinterBackend {
public:
    PrintJob render(const LabelDocument& doc, const PrinterProfile& profile) override;
};

} // namespace labelprint
