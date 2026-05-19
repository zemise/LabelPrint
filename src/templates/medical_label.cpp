#include "labelprint/template.h"

namespace labelprint {

namespace {

void addText(LabelDocument& doc,
             const MedicalLabelTextLayout& layout,
             const std::string& text) {
    doc.addText(layout.pos.x, layout.pos.y, text,
                layout.height, layout.width, layout.font);
}

} // namespace

LabelDocument buildMedicalLabel(const MedicalLabelData& data,
                                const LabelSettings& cfg) {
    MedicalLabelLayout layout;
    layout.settings = cfg;
    return buildMedicalLabel(data, layout);
}

LabelDocument buildMedicalLabel(const MedicalLabelData& data,
                                const MedicalLabelLayout& layout) {
    LabelDocument doc(layout.settings);

    // Row 1: Sample number (large, bold)
    addText(doc, layout.sampleNo, data.sampleNo);

    // Row 2: Test item name
    addText(doc, layout.testItem, data.testItem);

    // Barcode
    doc.addBarcode(layout.barcode.pos.x, layout.barcode.pos.y,
                   data.barcodeValue, layout.barcode.height,
                   layout.barcode.narrowWidth,
                   layout.barcode.printTextBelow);

    // Human-readable barcode text
    addText(doc, layout.barcodeText, data.barcodeValue);

    // Row 3: Patient name, specimen type, department
    addText(doc, layout.patientName, data.patientName);
    addText(doc, layout.specimenType, data.specimenType);
    if (!data.department.empty()) {
        addText(doc, layout.department, data.department);
    }

    // Row 4: Patient ID, timestamp
    addText(doc, layout.patientId, data.patientId);
    addText(doc, layout.timestamp, data.timestamp);

    return doc;
}

} // namespace labelprint
