#include "labelprint/template.h"

namespace labelprint {

LabelDocument buildMedicalLabel(const MedicalLabelData& data,
                                const LabelSettings& cfg) {
    LabelDocument doc(cfg);

    // Row 1: Sample number (large, bold)
    doc.addText(5, 5, data.sampleNo, 36, 22, Font::Bold);

    // Row 2: Test item name
    doc.addText(5, 44, data.testItem, 30, 18, Font::Medium);

    // Barcode
    doc.addBarcode(20, 72, data.barcodeValue, 75, 3, false);

    // Human-readable barcode text
    doc.addText(135, 152, data.barcodeValue, 18, 13, Font::Medium);

    // Row 3: Patient name, specimen type, department
    doc.addText(5, 175, data.patientName, 28, 22, Font::Medium);
    doc.addText(145, 175, data.specimenType, 26, 20, Font::Medium);
    if (!data.department.empty()) {
        doc.addText(245, 175, data.department, 26, 19, Font::Small);
    }

    // Row 4: Patient ID, timestamp
    doc.addText(5, 205, data.patientId, 20, 14, Font::Medium);
    doc.addText(245, 205, data.timestamp, 18, 11, Font::Small);

    return doc;
}

} // namespace labelprint
