#include "labelprint/zpl_backend.h"
#include "detail.h"
#include "zpl_label.h"

namespace labelprint {

PrintJob ZplBackend::render(const LabelDocument& doc, const PrinterProfile& profile) {
    zpl::LabelSettings zplSettings = detail::convertSettings(doc.settings(), profile);
    zpl::ZplLabel label(zplSettings);
    detail::populate(label, doc, profile);
    return PrintJob::fromText("zpl", label.generate());
}

} // namespace labelprint
