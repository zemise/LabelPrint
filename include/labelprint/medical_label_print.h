#pragma once

#include "labelprint/backend.h"
#include "labelprint/template.h"

#include <cstddef>
#include <string>

namespace labelprint {

enum class MedicalLabelPrinterModel {
    Auto,
    Unknown,
    XprinterXp360b,
    ZebraZd888
};

struct MedicalLabelPrintOptions {
    MedicalLabelPrinterModel model = MedicalLabelPrinterModel::Auto;
    MedicalLabelPrinterModel fallbackModel = MedicalLabelPrinterModel::XprinterXp360b;
    MedicalLabelLayout layout;
    bool useCustomLayout = false;
    int quantity = 1;
};

struct MedicalLabelPrintResult {
    MedicalLabelPrinterModel resolvedModel = MedicalLabelPrinterModel::Unknown;
    std::string profileName;
    std::string jobFormat;
    std::size_t bytes = 0;
};

std::string to_string(MedicalLabelPrinterModel model);

MedicalLabelPrinterModel detectMedicalLabelPrinterModel(const std::string& printerName);
MedicalLabelPrinterModel detectMedicalLabelPrinterModel(const std::wstring& printerName);

PrintJob renderMedicalLabel(const MedicalLabelData& data,
                            const MedicalLabelPrintOptions& options,
                            MedicalLabelPrinterModel* resolvedModel = nullptr);

MedicalLabelPrintResult printMedicalLabel(const std::string& printerName,
                                          const MedicalLabelData& data,
                                          const MedicalLabelPrintOptions& options = MedicalLabelPrintOptions{});

MedicalLabelPrintResult printMedicalLabel(const std::wstring& printerName,
                                          const MedicalLabelData& data,
                                          const MedicalLabelPrintOptions& options = MedicalLabelPrintOptions{});

} // namespace labelprint
