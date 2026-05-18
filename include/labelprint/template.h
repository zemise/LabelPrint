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
// Build a medical sample label from structured data.
// Positions and fonts match the verified XP-360B layout.
// ---------------------------------------------------------------------------
LabelDocument buildMedicalLabel(const MedicalLabelData& data,
                                const LabelSettings& cfg);

} // namespace labelprint
