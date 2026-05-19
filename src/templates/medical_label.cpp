#include "labelprint/template.h"

#include <algorithm>
#include <vector>

namespace labelprint {

namespace {

std::vector<std::string> splitUtf8Chars(const std::string& text) {
    std::vector<std::string> chars;
    for (size_t i = 0; i < text.size();) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        size_t len = 1;
        if (c >= 0xF0) {
            len = 4;
        } else if (c >= 0xE0) {
            len = 3;
        } else if (c >= 0xC0) {
            len = 2;
        }
        len = std::min(len, text.size() - i);
        chars.push_back(text.substr(i, len));
        i += len;
    }
    return chars;
}

std::vector<std::string> wrapText(const std::string& text,
                                  const MedicalLabelTextLayout& layout) {
    if (layout.maxWidth <= 0 || layout.maxLines <= 1 || layout.width <= 0) {
        return {text};
    }

    auto chars = splitUtf8Chars(text);
    int maxCharsPerLine = std::max(1, layout.maxWidth / layout.width);
    std::vector<std::string> lines;
    std::string current;
    int currentCount = 0;

    for (const auto& ch : chars) {
        if (currentCount >= maxCharsPerLine &&
            static_cast<int>(lines.size()) + 1 < layout.maxLines) {
            lines.push_back(current);
            current.clear();
            currentCount = 0;
        }
        current += ch;
        ++currentCount;
    }

    if (!current.empty() || lines.empty()) {
        lines.push_back(current);
    }

    if (static_cast<int>(lines.size()) > layout.maxLines) {
        lines.resize(layout.maxLines);
    }
    return lines;
}

void addText(LabelDocument& doc,
             const MedicalLabelTextLayout& layout,
             const std::string& text) {
    auto lines = wrapText(text, layout);
    for (size_t i = 0; i < lines.size(); ++i) {
        int y = layout.pos.y + static_cast<int>(i) * (layout.height + layout.lineGap);
        doc.addText(layout.pos.x, y, lines[i],
                    layout.height, layout.width, layout.font);
    }
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
                   layout.barcode.wideRatio,
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
