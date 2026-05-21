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

enum class MedicalLabelTextAlign {
    Left,
    Center
};

struct MedicalLabelTextLayout {
    MedicalLabelPoint pos;
    int height = 30;
    int width = 20;
    std::string font = Font::Default;
    int maxWidth = 0;       // optional, in dots; 0 means no wrapping
    int lineGap = 2;        // vertical gap between wrapped lines
    int maxLines = 1;       // maximum wrapped lines
    MedicalLabelTextAlign align = MedicalLabelTextAlign::Left;
};

struct MedicalLabelBarcodeLayout {
    MedicalLabelPoint pos;
    int height = 75;
    int narrowWidth = 2;
    double wideRatio = 2.0;
    bool printTextBelow = false;
};

struct MedicalLabelLayout {
    LabelSettings settings{400, 240, 5, 5, 25, 4, 1, true};
    int rowGap = 30;

    MedicalLabelTextLayout sampleNo{
        {5, 5}, 28, 16, Font::Medium, 0, 2, 1, MedicalLabelTextAlign::Left
    };
    MedicalLabelTextLayout testItem{
        {88, 8}, 22, 16, Font::Medium, 290, 2, 2, MedicalLabelTextAlign::Left
    };
    MedicalLabelBarcodeLayout barcode{
        {66, 72}, 75, 2, 3.0, false
    };
    MedicalLabelTextLayout barcodeText{
        {142, 152}, 18, 13, Font::Medium, 0, 2, 1, MedicalLabelTextAlign::Left
    };
    MedicalLabelTextLayout patientName{
        {5, 175}, 28, 22, Font::Medium, 0, 2, 1, MedicalLabelTextAlign::Left
    };
    MedicalLabelTextLayout specimenType{
        {145, 175}, 26, 20, Font::Medium, 0, 2, 1, MedicalLabelTextAlign::Left
    };
    MedicalLabelTextLayout department{
        {245, 175}, 26, 19, Font::Small, 0, 2, 1, MedicalLabelTextAlign::Left
    };
    MedicalLabelTextLayout patientId{
        {5, 205}, 20, 14, Font::Medium, 0, 2, 1, MedicalLabelTextAlign::Left
    };
    MedicalLabelTextLayout timestamp{
        {215, 205}, 18, 11, Font::Small, 0, 2, 1, MedicalLabelTextAlign::Left
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
