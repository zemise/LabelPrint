#pragma once

#include "labelprint/document.h"
#include <string>

namespace labelprint {

// ---------------------------------------------------------------------------
// MedicalLabelData — domain data for a medical sample label
// ---------------------------------------------------------------------------
struct MedicalLabelData {
    std::string  sampleNo;       // e.g. "22"
    std::string  testItem;       // e.g. u8"血常规（迈瑞流水线）"
    std::string  barcodeValue;   // e.g. "008085125"
    std::string  patientName;    // e.g. u8"廖明"
    std::string  specimenType;   // e.g. u8"全血"
    std::string  patientId;      // e.g. "202629988"
    std::string  timestamp;      // e.g. "2026/5/15 9:24"
    std::string  department;     // e.g. u8"心血管内科二区" (optional)
};

// ---------------------------------------------------------------------------
// MedicalLabelLayout — configurable layout for buildMedicalLabel().
// Defaults match the verified XP-360B 50x30 mm layout.
// ---------------------------------------------------------------------------
struct MedicalLabelPoint {
    int x = 0;
    int y = 0;
};

struct MedicalLabelTextLayout {
    MedicalLabelPoint pos;
    int height = 30;
    int width = 20;
    std::string font = Font::Default;
};

struct MedicalLabelBarcodeLayout {
    MedicalLabelPoint pos;
    int height = 75;
    int narrowWidth = 3;
    bool printTextBelow = false;
};

struct MedicalLabelLayout {
    LabelSettings settings{400, 240, 5, 5, 25, 4, 1, true};
    int rowGap = 30;

    MedicalLabelTextLayout sampleNo{
        {5, 5}, 36, 22, Font::Bold
    };
    MedicalLabelTextLayout testItem{
        {5, 44}, 30, 18, Font::Medium
    };
    MedicalLabelBarcodeLayout barcode{
        {20, 72}, 75, 3, false
    };
    MedicalLabelTextLayout barcodeText{
        {135, 152}, 18, 13, Font::Medium
    };
    MedicalLabelTextLayout patientName{
        {5, 175}, 28, 22, Font::Medium
    };
    MedicalLabelTextLayout specimenType{
        {145, 175}, 26, 20, Font::Medium
    };
    MedicalLabelTextLayout department{
        {245, 175}, 26, 19, Font::Small
    };
    MedicalLabelTextLayout patientId{
        {5, 205}, 20, 14, Font::Medium
    };
    MedicalLabelTextLayout timestamp{
        {245, 205}, 18, 11, Font::Small
    };
};

// ---------------------------------------------------------------------------
// Build a medical sample label from structured data.
// Positions and fonts match the verified XP-360B layout.
// ---------------------------------------------------------------------------
LabelDocument buildMedicalLabel(const MedicalLabelData& data,
                                const LabelSettings& cfg);

LabelDocument buildMedicalLabel(const MedicalLabelData& data,
                                const MedicalLabelLayout& layout);

} // namespace labelprint
