#include "labelprint/labelprint.h"

#include <iostream>
#include <string>

namespace {

std::string argValue(int argc, char* argv[], const std::string& key,
                     const std::string& fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == key) {
            return argv[i + 1];
        }
    }
    return fallback;
}

bool hasFlag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == flag) {
            return true;
        }
    }
    return false;
}

const labelprint::PrinterProfile& selectProfile(const std::string& name) {
    if (name == "zebra_zd888") {
        return labelprint::PrinterProfiles::zebra_zd888();
    }
    return labelprint::PrinterProfiles::xprinter_xp360b();
}

labelprint::LabelSettings buildSettings(const labelprint::PrinterProfile& profile) {
    labelprint::LabelSettings cfg;
    cfg.width      = 400;
    cfg.height     = 240;
    cfg.homeX      = 5;
    cfg.homeY      = 5;
    cfg.darkness   = profile.darkness;
    cfg.printSpeed = profile.speed;
    cfg.quantity   = 1;
    return cfg;
}

labelprint::MedicalLabelData buildAsciiData() {
    labelprint::MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "CBC PANEL";
    data.barcodeValue = "008085125";
    data.patientName  = "LIAO MING";
    data.specimenType = "WHOLE BLOOD";
    data.department   = "CARDIOLOGY-2";
    data.patientId    = "202629988";
    data.timestamp    = "2026/5/15 9:24";
    return data;
}

labelprint::MedicalLabelData buildCnData() {
    labelprint::MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = u8"血常规（迈瑞流水线）";
    data.barcodeValue = "008085125";
    data.patientName  = u8"廖明";
    data.specimenType = u8"全血";
    data.department   = u8"心血管内科二区";
    data.patientId    = "202629988";
    data.timestamp    = "2026/5/15 9:24";
    return data;
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace labelprint;

    const auto& profile = selectProfile(argValue(argc, argv, "--profile", "xprinter_xp360b"));
    const bool asciiOnly = hasFlag(argc, argv, "--ascii-only");

    LabelSettings cfg = buildSettings(profile);

    std::cout << "// Printer profile: " << profile.name
              << " (" << profile.dpi << " DPI, "
              << (profile.language == PrinterLanguage::TSPL ? "TSPL" : "ZPL") << ")"
              << std::endl;

    FileTransport fileTransport;
    LabelDocument asciiDoc = buildMedicalLabel(buildAsciiData(), cfg);

    if (profile.language == PrinterLanguage::ZPL) {
        ZplBackend zplBackend;
        LabelDocument zplDoc = asciiOnly ? asciiDoc : buildMedicalLabel(buildCnData(), cfg);
        PrintJob zplJob = zplBackend.render(zplDoc, profile);
        std::cout << zplJob.asText() << std::endl;
        fileTransport.send(zplJob, PrinterConnection{"label_medical.zpl"});
        std::cout << "\n// ZPL saved to label_medical.zpl" << std::endl;
        return 0;
    }

    TsplBackend tsplBackend;
    PrintJob tsplJob = tsplBackend.render(asciiDoc, profile);
    std::cout << "\n// TSPL preview\n" << tsplJob.asText() << std::endl;
    fileTransport.send(tsplJob, PrinterConnection{"label_medical.tspl"});
    std::cout << "// TSPL saved to label_medical.tspl" << std::endl;

    if (asciiOnly) {
        return 0;
    }

    MedicalLabelPrintOptions cnOptions;
    cnOptions.model = MedicalLabelPrinterModel::XprinterXp360b;
    cnOptions.quantity = cfg.quantity;
    PrintJob cnJob = renderMedicalLabel(buildCnData(), cnOptions);
    std::cout << "\n// Chinese TSPL (GB18030) preview\n"
              << cnJob.debugText << std::endl;
    fileTransport.send(cnJob, PrinterConnection{"label_medical_cn_new.prn"});
    std::cout << "// Chinese TSPL saved to label_medical_cn_new.prn" << std::endl;

    return 0;
}
