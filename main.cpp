#include "labelprint/labelprint.h"
#include <iostream>

int main() {
    using namespace labelprint;

    const auto& profile = PrinterProfiles::xprinter_xp360b();

    LabelSettings cfg;
    cfg.width      = 400;
    cfg.height     = 240;
    cfg.homeX      = 5;
    cfg.homeY      = 5;
    cfg.darkness   = profile.darkness;
    cfg.printSpeed = profile.speed;
    cfg.quantity   = 1;

    std::cout << "// Printer: " << profile.name
              << " (" << profile.dpi << " DPI, "
              << (profile.language == PrinterLanguage::TSPL ? "TSPL" : "ZPL") << ")"
              << std::endl;

    // === Transports ===
    FileTransport fileTransport;

    // === ASCII label ===
    MedicalLabelData ascii;
    ascii.sampleNo     = "22";
    ascii.testItem     = "CBC PANEL";
    ascii.barcodeValue = "008085125";
    ascii.patientName  = "LIAO MING";
    ascii.specimenType = "WHOLE BLOOD";
    ascii.patientId    = "202629988";
    ascii.timestamp    = "2026/5/15 9:24";

    LabelDocument doc = buildMedicalLabel(ascii, cfg);

    ZplBackend zplBackend;
    TsplBackend tsplBackend;

    PrintJob zplJob = zplBackend.render(doc, profile);
    std::cout << zplJob.asText() << std::endl;
    fileTransport.send(zplJob, PrinterConnection{"label_medical.zpl"});
    std::cout << "\n// ZPL saved to label_medical.zpl" << std::endl;

    PrintJob tsplJob = tsplBackend.render(doc, profile);
    std::cout << "\n// TSPL preview\n" << tsplJob.asText() << std::endl;
    fileTransport.send(tsplJob, PrinterConnection{"label_medical.tspl"});
    std::cout << "// TSPL saved to label_medical.tspl" << std::endl;

    // Preview
    std::cout << "// Generating local preview..." << std::endl;
    std::system("python3 preview_render.py --scale 3 --output label_medical.png 2>&1");
    std::cout << "// Preview saved to label_medical.png" << std::endl;

    // === Chinese label ===
    MedicalLabelData cn;
    cn.sampleNo     = "22";
    cn.testItem     = u8"血常规（迈瑞流水线）";
    cn.barcodeValue = "008085125";
    cn.patientName  = u8"廖明";
    cn.specimenType = u8"全血";
    cn.department   = u8"心血管内科二区";
    cn.patientId    = "202629988";
    cn.timestamp    = "2026/5/15 9:24";

    LabelDocument cnDoc = buildMedicalLabel(cn, cfg);

    TsplBitmapBackend bitmapBackend;
    PrintJob cnJob = bitmapBackend.render(cnDoc, profile);
    std::cout << "\n// Chinese TSPL (bitmap) preview\n"
              << cnJob.debugText << std::endl;
    fileTransport.send(cnJob, PrinterConnection{"label_medical_cn_new.prn"});
    std::cout << "// Chinese TSPL saved to label_medical_cn_new.prn" << std::endl;

    // Uncomment to print directly:
    // WindowsRawTransport rawTransport;
    // rawTransport.send(cnJob, PrinterConnection{"Xprinter XP-360B #2"});

    return 0;
}
