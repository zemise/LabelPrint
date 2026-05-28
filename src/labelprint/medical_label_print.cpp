#include "labelprint/medical_label_print.h"

#include "labelprint/printer_profile.h"
#include "labelprint/transport.h"
#include "labelprint/ezpl_gb2312_backend.h"
#include "labelprint/tspl_bitmap_backend.h"
#include "labelprint/tspl_gb18030_backend.h"
#include "labelprint/zpl_backend.h"

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winspool.h>
#endif

namespace labelprint {

namespace {

std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::wstring lowerWide(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return value;
}

bool containsAny(const std::string& haystack, const std::vector<std::string>& needles) {
    for (const auto& needle : needles) {
        if (haystack.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsAny(const std::wstring& haystack, const std::vector<std::wstring>& needles) {
    for (const auto& needle : needles) {
        if (haystack.find(needle) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

void appendField(std::string& target, const char* value) {
    if (value && *value) {
        target += ' ';
        target += value;
    }
}

void appendField(std::wstring& target, const wchar_t* value) {
    if (value && *value) {
        target += L' ';
        target += value;
    }
}

#ifdef _WIN32
void appendWindowsPrinterInfo(const std::string& printerName, std::string& probeText) {
    if (printerName.empty()) {
        return;
    }

    HANDLE printer = nullptr;
    if (!OpenPrinterA(const_cast<char*>(printerName.c_str()), &printer, nullptr)) {
        return;
    }

    DWORD needed = 0;
    GetPrinterA(printer, 2, nullptr, 0, &needed);
    if (needed == 0) {
        ClosePrinter(printer);
        return;
    }

    std::vector<BYTE> buffer(needed);
    if (GetPrinterA(printer, 2, buffer.data(), needed, &needed)) {
        auto* info = reinterpret_cast<PRINTER_INFO_2A*>(buffer.data());
        appendField(probeText, info->pPrinterName);
        appendField(probeText, info->pDriverName);
        appendField(probeText, info->pPortName);
        appendField(probeText, info->pPrintProcessor);
        appendField(probeText, info->pDatatype);
        appendField(probeText, info->pComment);
        appendField(probeText, info->pLocation);
        appendField(probeText, info->pShareName);
    }

    ClosePrinter(printer);
}

void appendWindowsPrinterInfo(const std::wstring& printerName, std::wstring& probeText) {
    if (printerName.empty()) {
        return;
    }

    HANDLE printer = nullptr;
    if (!OpenPrinterW(const_cast<wchar_t*>(printerName.c_str()), &printer, nullptr)) {
        return;
    }

    DWORD needed = 0;
    GetPrinterW(printer, 2, nullptr, 0, &needed);
    if (needed == 0) {
        ClosePrinter(printer);
        return;
    }

    std::vector<BYTE> buffer(needed);
    if (GetPrinterW(printer, 2, buffer.data(), needed, &needed)) {
        auto* info = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
        appendField(probeText, info->pPrinterName);
        appendField(probeText, info->pDriverName);
        appendField(probeText, info->pPortName);
        appendField(probeText, info->pPrintProcessor);
        appendField(probeText, info->pDatatype);
        appendField(probeText, info->pComment);
        appendField(probeText, info->pLocation);
        appendField(probeText, info->pShareName);
    }

    ClosePrinter(printer);
}
#endif

MedicalLabelPrinterModel modelFromText(const std::string& text) {
    const std::string probe = lowerAscii(text);
    if (containsAny(probe, {"godex", "g500u", "g500-u", "g500", "g-500", "gzpl"})) {
        return MedicalLabelPrinterModel::GodexG500u;
    }
    if (containsAny(probe, {"zebra", "zd888", "zd-888", "zdesigner", " zpl", "zpl ", "zpl-"})) {
        return MedicalLabelPrinterModel::ZebraZd888;
    }
    if (containsAny(probe, {"xprinter", "xp-360", "xp360", "xp 360", "tspl", "tsp"})) {
        return MedicalLabelPrinterModel::XprinterXp360b;
    }
    return MedicalLabelPrinterModel::Unknown;
}

MedicalLabelPrinterModel modelFromText(const std::wstring& text) {
    const std::wstring probe = lowerWide(text);
    if (containsAny(probe, {L"godex", L"g500u", L"g500-u", L"g500", L"g-500", L"gzpl"})) {
        return MedicalLabelPrinterModel::GodexG500u;
    }
    if (containsAny(probe, {L"zebra", L"zd888", L"zd-888", L"zdesigner", L" zpl", L"zpl ", L"zpl-"})) {
        return MedicalLabelPrinterModel::ZebraZd888;
    }
    if (containsAny(probe, {L"xprinter", L"xp-360", L"xp360", L"xp 360", L"tspl", L"tsp"})) {
        return MedicalLabelPrinterModel::XprinterXp360b;
    }
    return MedicalLabelPrinterModel::Unknown;
}

MedicalLabelPrinterModel normalizeFallback(MedicalLabelPrinterModel fallbackModel) {
    if (fallbackModel == MedicalLabelPrinterModel::Auto ||
        fallbackModel == MedicalLabelPrinterModel::Unknown) {
        return MedicalLabelPrinterModel::XprinterXp360b;
    }
    return fallbackModel;
}

MedicalLabelPrinterModel resolveModel(const std::string& printerName,
                                      const MedicalLabelPrintOptions& options) {
    if (options.model != MedicalLabelPrinterModel::Auto) {
        if (options.model == MedicalLabelPrinterModel::Unknown) {
            return normalizeFallback(options.fallbackModel);
        }
        return options.model;
    }

    const MedicalLabelPrinterModel detected = detectMedicalLabelPrinterModel(printerName);
    if (detected != MedicalLabelPrinterModel::Unknown &&
        detected != MedicalLabelPrinterModel::Auto) {
        return detected;
    }
    return normalizeFallback(options.fallbackModel);
}

const PrinterProfile& profileFor(MedicalLabelPrinterModel model) {
    switch (model) {
    case MedicalLabelPrinterModel::GodexG500u:
        return PrinterProfiles::godex_g500u();
    case MedicalLabelPrinterModel::ZebraZd888:
        return PrinterProfiles::zebra_zd888();
    case MedicalLabelPrinterModel::XprinterXp360b:
        return PrinterProfiles::xprinter_xp360b();
    case MedicalLabelPrinterModel::Auto:
    case MedicalLabelPrinterModel::Unknown:
    default:
        return PrinterProfiles::xprinter_xp360b();
    }
}

MedicalLabelLayout effectiveLayout(const MedicalLabelPrintOptions& options,
                                   MedicalLabelPrinterModel model,
                                   const PrinterProfile& profile) {
    MedicalLabelLayout layout = options.useCustomLayout ? options.layout : MedicalLabelLayout{};
    if (!options.useCustomLayout && model == MedicalLabelPrinterModel::XprinterXp360b) {
        layout.barcode.pos.x = 36;
        layout.barcode.narrowWidth = 3;
        layout.barcode.wideRatio = 2.6;
        layout.barcodeText.pos.x = 0;
        layout.barcodeText.maxWidth = layout.settings.width - 60;
        layout.barcodeText.align = MedicalLabelTextAlign::Center;
        layout.testItem.height = 18;
        layout.testItem.width = 13;
        layout.patientName.height = 14;
        layout.patientName.width = 11;
        layout.specimenType.height = 13;
        layout.specimenType.width = 10;
        layout.department.height = 13;
        layout.department.width = 10;
        layout.patientId.height = 16;
        layout.patientId.width = 11;
        layout.timestamp.height = 15;
        layout.timestamp.width = 9;
    } else if (!options.useCustomLayout && model == MedicalLabelPrinterModel::ZebraZd888) {
        layout.sampleNo.height = 32;
        layout.sampleNo.width = 18;
        layout.testItem.height = 26;
        layout.testItem.width = 18;
        layout.barcodeText.height = 20;
        layout.barcodeText.width = 14;
        layout.patientName.height = 32;
        layout.patientName.width = 24;
        layout.specimenType.height = 30;
        layout.specimenType.width = 22;
        layout.department.height = 28;
        layout.department.width = 20;
        layout.patientId.height = 22;
        layout.patientId.width = 16;
        layout.timestamp.height = 20;
        layout.timestamp.width = 12;
    } else if (!options.useCustomLayout && model == MedicalLabelPrinterModel::GodexG500u) {
        layout.sampleNo.pos.x = 13; // + homeX 5 => x=18
        layout.sampleNo.height = 28;
        layout.sampleNo.width = 16;
        layout.testItem.height = 22;
        layout.testItem.width = 16;
        layout.barcode.pos.x = 57; // + homeX 5 => x=62
        layout.barcode.narrowWidth = 3;
        layout.barcode.wideRatio = 2.5; // EZPL BQ wide=7
        layout.barcodeText.height = 18;
        layout.barcodeText.width = 13;
        layout.patientName.pos.x = 13; // + homeX 5 => x=18
        layout.patientName.height = 24;
        layout.patientName.width = 22;
        layout.specimenType.height = 24;
        layout.specimenType.width = 20;
        layout.department.height = 24;
        layout.department.width = 19;
        layout.patientId.pos.x = 13; // + homeX 5 => x=18
        layout.patientId.height = 20;
        layout.patientId.width = 14;
        layout.timestamp.height = 18;
        layout.timestamp.width = 11;
    }
    layout.settings.darkness = profile.darkness;
    layout.settings.printSpeed = profile.speed;
    if (options.quantity > 0) {
        layout.settings.quantity = options.quantity;
    }
    return layout;
}

PrintJob renderWithModel(const LabelDocument& doc,
                         const PrinterProfile& profile,
                         MedicalLabelPrinterModel model) {
    if (model == MedicalLabelPrinterModel::GodexG500u) {
        EzplGb2312Backend backend;
        return backend.render(doc, profile);
    }
    if (model == MedicalLabelPrinterModel::ZebraZd888) {
        ZplBackend backend;
        return backend.render(doc, profile);
    }

    TsplGb18030Backend backend;
    return backend.render(doc, profile);
}

} // namespace

std::string to_string(MedicalLabelPrinterModel model) {
    switch (model) {
    case MedicalLabelPrinterModel::Auto:
        return "auto";
    case MedicalLabelPrinterModel::Unknown:
        return "unknown";
    case MedicalLabelPrinterModel::XprinterXp360b:
        return "xprinter-xp360b";
    case MedicalLabelPrinterModel::ZebraZd888:
        return "zebra-zd888";
    case MedicalLabelPrinterModel::GodexG500u:
        return "godex-g500u";
    }
    return "unknown";
}

MedicalLabelPrinterModel detectMedicalLabelPrinterModel(const std::string& printerName) {
    std::string probeText = printerName;
#ifdef _WIN32
    appendWindowsPrinterInfo(printerName, probeText);
#endif
    return modelFromText(probeText);
}

MedicalLabelPrinterModel detectMedicalLabelPrinterModel(const std::wstring& printerName) {
    std::wstring probeText = printerName;
#ifdef _WIN32
    appendWindowsPrinterInfo(printerName, probeText);
#endif
    return modelFromText(probeText);
}

PrintJob renderMedicalLabel(const MedicalLabelData& data,
                            const MedicalLabelPrintOptions& options,
                            MedicalLabelPrinterModel* resolvedModel) {
    const MedicalLabelPrinterModel model = normalizeFallback(options.model == MedicalLabelPrinterModel::Auto
                                                                 ? options.fallbackModel
                                                                 : options.model);
    const PrinterProfile& profile = profileFor(model);
    const MedicalLabelLayout layout = effectiveLayout(options, model, profile);
    LabelDocument doc = buildMedicalLabel(data, layout);
    if (resolvedModel) {
        *resolvedModel = model;
    }
    return renderWithModel(doc, profile, model);
}

MedicalLabelPrintResult printMedicalLabel(const std::string& printerName,
                                          const MedicalLabelData& data,
                                          const MedicalLabelPrintOptions& options) {
    if (printerName.empty()) {
        throw std::runtime_error("Printer name is required");
    }

    const MedicalLabelPrinterModel model = resolveModel(printerName, options);
    const PrinterProfile& profile = profileFor(model);
    const MedicalLabelLayout layout = effectiveLayout(options, model, profile);
    LabelDocument doc = buildMedicalLabel(data, layout);
    PrintJob job = renderWithModel(doc, profile, model);

    WindowsRawTransport transport;
    transport.send(job, PrinterConnection{printerName});

    MedicalLabelPrintResult result;
    result.resolvedModel = model;
    result.profileName = profile.name;
    result.jobFormat = job.format;
    result.bytes = job.data.size();
    return result;
}

MedicalLabelPrintResult printMedicalLabel(const std::wstring& printerName,
                                          const MedicalLabelData& data,
                                          const MedicalLabelPrintOptions& options) {
    if (printerName.empty()) {
        throw std::runtime_error("Printer name is required");
    }

    const MedicalLabelPrinterModel model = options.model == MedicalLabelPrinterModel::Auto
                                               ? detectMedicalLabelPrinterModel(printerName)
                                               : options.model;
    MedicalLabelPrintOptions resolvedOptions = options;
    resolvedOptions.model = model == MedicalLabelPrinterModel::Unknown ||
                                    model == MedicalLabelPrinterModel::Auto
                                ? normalizeFallback(options.fallbackModel)
                                : model;

    MedicalLabelPrinterModel resolvedModel = MedicalLabelPrinterModel::Unknown;
    PrintJob job = renderMedicalLabel(data, resolvedOptions, &resolvedModel);
    const PrinterProfile& profile = profileFor(resolvedModel);

    PrinterConnection conn;
    conn.wideName = printerName;
    WindowsRawTransport transport;
    transport.send(job, conn);

    MedicalLabelPrintResult result;
    result.resolvedModel = resolvedModel;
    result.profileName = profile.name;
    result.jobFormat = job.format;
    result.bytes = job.data.size();
    return result;
}

} // namespace labelprint
