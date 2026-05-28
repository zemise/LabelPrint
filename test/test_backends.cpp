// Backend output tests — verify ZPL, TSPL, and EZPL contain expected commands
#include "test_utils.h"
#include "labelprint/document.h"
#include "labelprint/printer_profile.h"
#include "labelprint/backend.h"
#include "labelprint/ezpl_gb2312_backend.h"
#include "labelprint/medical_label_print.h"
#include "labelprint/template.h"
#include "labelprint/transport.h"
#include "labelprint/zpl_backend.h"
#include "labelprint/tspl_backend.h"
#include "labelprint/tspl_bitmap_backend.h"
#include <algorithm>
#include <string>

using namespace labelprint;

// ---------------------------------------------------------------------------
// Helpers: build a minimal label for testing
// ---------------------------------------------------------------------------
static LabelSettings testSettings() {
    LabelSettings cfg;
    cfg.width      = 400;
    cfg.height     = 240;
    cfg.homeX      = 5;
    cfg.homeY      = 5;
    cfg.darkness   = 25;
    cfg.printSpeed = 4;
    cfg.quantity   = 1;
    return cfg;
}

static LabelDocument makeDoc() {
    LabelDocument doc(testSettings());
    doc.addText(5, 5, "TEST", 30, 20, Font::Medium);
    doc.addBarcode(20, 72, "123456", 75, 3, 3.0, false);
    return doc;
}

// ---------------------------------------------------------------------------
// ZPL backend tests
// ---------------------------------------------------------------------------
ADD_TEST(zpl_backend_is_registered) {
    ZplBackend backend;
    // Just verify it can be instantiated
}

ADD_TEST(zpl_output_contains_start_end) {
    auto doc = makeDoc();
    const auto& profile = PrinterProfiles::zebra_zd888();
    ZplBackend backend;
    PrintJob job = backend.render(doc, profile);
    std::string zpl = job.asText();
    ASSERT(zpl.find("^XA") != std::string::npos);
    ASSERT(zpl.find("^XZ") != std::string::npos);
    ASSERT_EQ(job.format, "zpl");
}

ADD_TEST(zpl_output_contains_text_command) {
    auto doc = makeDoc();
    const auto& profile = PrinterProfiles::zebra_zd888();
    ZplBackend backend;
    PrintJob job = backend.render(doc, profile);
    std::string zpl = job.asText();
    ASSERT(zpl.find("^FD") != std::string::npos);
    ASSERT(zpl.find("TEST") != std::string::npos);
}

ADD_TEST(zpl_output_contains_barcode_command) {
    auto doc = makeDoc();
    const auto& profile = PrinterProfiles::zebra_zd888();
    ZplBackend backend;
    PrintJob job = backend.render(doc, profile);
    std::string zpl = job.asText();
    ASSERT(zpl.find("^BC") != std::string::npos);
    ASSERT(zpl.find("123456") != std::string::npos);
}

ADD_TEST(zpl_output_contains_label_settings) {
    auto doc = makeDoc();
    const auto& profile = PrinterProfiles::zebra_zd888();
    ZplBackend backend;
    PrintJob job = backend.render(doc, profile);
    std::string zpl = job.asText();
    ASSERT(zpl.find("^PW400") != std::string::npos);
    ASSERT(zpl.find("^LL240") != std::string::npos);
    ASSERT(zpl.find("~SD25") != std::string::npos);
}

ADD_TEST(zpl_cjk_text_uses_profile_native_font) {
    LabelDocument doc(testSettings());
    doc.addText(5, 5, u8"中文测试", 30, 20, Font::Medium);

    const auto& profile = PrinterProfiles::zebra_zd888();
    ZplBackend backend;
    PrintJob job = backend.render(doc, profile);
    std::string zpl = job.asText();

    ASSERT(zpl.find("^A@N,30,20,E:CSONG.TTF") != std::string::npos);
    ASSERT(zpl.find(u8"中文测试") != std::string::npos);
}

// ---------------------------------------------------------------------------
// TSPL backend tests
// ---------------------------------------------------------------------------
ADD_TEST(tspl_backend_is_registered) {
    TsplBackend backend;
}

ADD_TEST(tspl_output_contains_size_gap) {
    auto doc = makeDoc();
    const auto& profile = PrinterProfiles::xprinter_xp360b();
    TsplBackend backend;
    PrintJob job = backend.render(doc, profile);
    std::string tspl = job.asText();
    ASSERT(tspl.find("SIZE") != std::string::npos);
    ASSERT(tspl.find("GAP") != std::string::npos);
    ASSERT(tspl.find("DENSITY") != std::string::npos);
    ASSERT(tspl.find("PRINT") != std::string::npos);
    ASSERT_EQ(job.format, "tspl");
}

ADD_TEST(tspl_output_contains_text) {
    auto doc = makeDoc();
    const auto& profile = PrinterProfiles::xprinter_xp360b();
    TsplBackend backend;
    PrintJob job = backend.render(doc, profile);
    std::string tspl = job.asText();
    ASSERT(tspl.find("TEXT") != std::string::npos);
    ASSERT(tspl.find("TEST") != std::string::npos);
}

ADD_TEST(tspl_output_contains_barcode) {
    auto doc = makeDoc();
    const auto& profile = PrinterProfiles::xprinter_xp360b();
    TsplBackend backend;
    PrintJob job = backend.render(doc, profile);
    std::string tspl = job.asText();
    ASSERT(tspl.find("BARCODE") != std::string::npos);
    ASSERT(tspl.find("123456") != std::string::npos);
}

// ---------------------------------------------------------------------------
// TsplBitmapBackend tests
// ---------------------------------------------------------------------------
ADD_TEST(tspl_bitmap_backend_registered) {
    TsplBitmapBackend backend;
}

ADD_TEST(stable_public_api_names_are_available) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "CBC";
    data.barcodeValue = "123";
    data.patientName  = "ABC";
    data.specimenType = "BLOOD";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    LabelSettings settings = testSettings();
    LabelDocument doc = buildMedicalLabel(data, settings);

    TsplBitmapBackend backend;
    IPrinterBackend* backendApi = &backend;

    WindowsRawTransport raw;
    IPrintTransport* transportApi = &raw;

    ASSERT_EQ(doc.settings().width, settings.width);
    ASSERT(backendApi != nullptr);
    ASSERT(transportApi != nullptr);
}

ADD_TEST(tspl_bitmap_ascii_text_uses_native) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "HELLO";
    data.barcodeValue = "123";
    data.patientName  = "ABC";
    data.specimenType = "X";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    LabelDocument cnDoc = buildMedicalLabel(data, testSettings());
    const auto& profile = PrinterProfiles::xprinter_xp360b();
    TsplBitmapBackend backend;
    PrintJob job = backend.render(cnDoc, profile);

    // ASCII text should use native TEXT commands (not be bitmap-rendered)
    std::string debug = job.debugText;
    ASSERT(debug.find("TEXT") != std::string::npos);
    ASSERT(debug.find("HELLO") != std::string::npos);
    // No BITMAP annotation expected for pure ASCII content
    ASSERT(debug.find("(BITMAP") == std::string::npos);
    ASSERT_EQ(job.format, "tspl-bitmap");
}

ADD_TEST(tspl_bitmap_cjk_text_uses_bitmap) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = u8"血常规";
    data.barcodeValue = "123";
    data.patientName  = "ABC";
    data.specimenType = "X";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    LabelDocument cnDoc = buildMedicalLabel(data, testSettings());
    const auto& profile = PrinterProfiles::xprinter_xp360b();
    TsplBitmapBackend backend;
    PrintJob job = backend.render(cnDoc, profile);

    // CJK text should produce BITMAP annotations in debug text
    std::string debug = job.debugText;
    ASSERT(debug.find("(BITMAP") != std::string::npos);
}

ADD_TEST(medical_label_layout_can_override_settings) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "CBC";
    data.barcodeValue = "123";
    data.patientName  = "ABC";
    data.specimenType = "BLOOD";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    MedicalLabelLayout layout;
    layout.settings.width = 320;
    layout.settings.height = 180;
    layout.settings.homeX = 12;
    layout.settings.homeY = 14;

    LabelDocument doc = buildMedicalLabel(data, layout);

    ASSERT_EQ(doc.settings().width, 320);
    ASSERT_EQ(doc.settings().height, 180);
    ASSERT_EQ(doc.settings().homeX, 12);
    ASSERT_EQ(doc.settings().homeY, 14);
}

ADD_TEST(medical_label_layout_can_override_fields) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "CBC";
    data.barcodeValue = "123";
    data.patientName  = "ABC";
    data.specimenType = "BLOOD";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";
    data.department   = "WARD";

    MedicalLabelLayout layout;
    layout.rowGap = 28;
    layout.sampleNo = {{11, 12}, 40, 24, Font::Bold};
    layout.barcode = {{31, 82}, 90, 4, 2.0, true};
    layout.patientName = {{9, 160}, 24, 18, Font::Medium};
    layout.timestamp = {{210, 198}, 16, 10, Font::Small};

    LabelDocument doc = buildMedicalLabel(data, layout);

    ASSERT_EQ(layout.rowGap, 28);
    ASSERT_EQ(doc.texts()[0].x, 11);
    ASSERT_EQ(doc.texts()[0].y, 12);
    ASSERT_EQ(doc.texts()[0].height, 40);
    ASSERT_EQ(doc.texts()[0].width, 24);
    ASSERT_EQ(doc.barcodes()[0].x, 31);
    ASSERT_EQ(doc.barcodes()[0].y, 82);
    ASSERT_EQ(doc.barcodes()[0].height, 90);
    ASSERT_EQ(doc.barcodes()[0].narrowWidth, 4);
    ASSERT_EQ(doc.barcodes()[0].printTextBelow, true);
    ASSERT_EQ(doc.texts()[3].x, 9);
    ASSERT_EQ(doc.texts()[3].y, 160);
    ASSERT_EQ(doc.texts()[7].x, 210);
    ASSERT_EQ(doc.texts()[7].y, 198);
}

ADD_TEST(medical_label_wraps_test_item) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "ABCDEFGHIJK";
    data.barcodeValue = "123";
    data.patientName  = "ABC";
    data.specimenType = "BLOOD";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    MedicalLabelLayout layout;
    layout.testItem.pos = {88, 8};
    layout.testItem.height = 22;
    layout.testItem.width = 16;
    layout.testItem.maxWidth = 80;
    layout.testItem.maxLines = 2;
    layout.testItem.lineGap = 3;

    LabelDocument doc = buildMedicalLabel(data, layout);

    ASSERT_EQ(doc.texts()[0].x, 5);
    ASSERT_EQ(doc.texts()[1].x, 88);
    ASSERT_EQ(doc.texts()[1].y, 8);
    ASSERT_EQ(doc.texts()[2].x, 88);
    ASSERT_EQ(doc.texts()[2].y, 33);

    MedicalLabelLayout defaultLayout;
    LabelDocument defaultDoc = buildMedicalLabel(data, defaultLayout);

    ASSERT_EQ(defaultDoc.texts()[0].height, 28);
    ASSERT_EQ(defaultDoc.texts()[0].width, 16);
    ASSERT_EQ(defaultDoc.texts()[1].height, 22);
    ASSERT_EQ(defaultDoc.texts()[1].width, 16);
    ASSERT_EQ(defaultDoc.texts()[1].x, 88);
    ASSERT_EQ(defaultDoc.texts()[3].height, 28);
    ASSERT_EQ(defaultDoc.texts()[3].width, 22);
    ASSERT_EQ(defaultDoc.barcodes()[0].narrowWidth, 2);
}

ADD_TEST(xprinter_default_print_layout_widens_and_centers_barcode) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "CBC";
    data.barcodeValue = "008085125";
    data.patientName  = "ABC";
    data.specimenType = "BLOOD";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    MedicalLabelPrintOptions options;
    options.model = MedicalLabelPrinterModel::XprinterXp360b;

    PrintJob job = renderMedicalLabel(data, options);

    ASSERT_EQ(job.format, "tspl-gb18030");
    ASSERT(job.debugText.find("CODEPAGE 54936") != std::string::npos);
    ASSERT(job.debugText.find("BARCODE 36,72,\"128\",75,0,0,3,7,\"008085125\"") != std::string::npos);
    ASSERT(job.debugText.find("TEXT 111,152") != std::string::npos);
    ASSERT(job.debugText.find("TEXT 5,175,\"3\",0,1,1,\"ABC\"") != std::string::npos);
    ASSERT(job.debugText.find("TEXT 145,175,\"3\",0,1,1,\"BLOOD\"") != std::string::npos);
    ASSERT(job.debugText.find("\"008085125\"") != std::string::npos);
}

ADD_TEST(xprinter_gb18030_backend_encodes_chinese_text) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = u8"血常规";
    data.barcodeValue = "123";
    data.patientName  = u8"中文测试";
    data.specimenType = u8"全血";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    MedicalLabelPrintOptions options;
    options.model = MedicalLabelPrinterModel::XprinterXp360b;

    PrintJob job = renderMedicalLabel(data, options);

    const std::vector<uint8_t> gb18030ChineseTest = {
        0xD6, 0xD0, 0xCE, 0xC4, 0xB2, 0xE2, 0xCA, 0xD4
    };
    const auto found = std::search(job.data.begin(), job.data.end(),
                                   gb18030ChineseTest.begin(),
                                   gb18030ChineseTest.end());

    ASSERT_EQ(job.format, "tspl-gb18030");
    ASSERT(found != job.data.end());
}

ADD_TEST(zebra_default_print_layout_uses_larger_text) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "CBC";
    data.barcodeValue = "008085125";
    data.patientName  = "ABC";
    data.specimenType = "BLOOD";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    MedicalLabelPrintOptions options;
    options.model = MedicalLabelPrinterModel::ZebraZd888;

    PrintJob job = renderMedicalLabel(data, options);
    std::string zpl = job.asText();

    ASSERT_EQ(job.format, "zpl");
    ASSERT(zpl.find("^ACN,32,24^FDABC") != std::string::npos);
    ASSERT(zpl.find("^ACN,30,22^FDBLOOD") != std::string::npos);
    ASSERT(zpl.find("^ACN,26,18^FDCBC") != std::string::npos);
}

ADD_TEST(godex_g500u_uses_ezpl_gb2312_path) {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "CBC";
    data.barcodeValue = "008085125";
    data.patientName  = "ABC";
    data.specimenType = "BLOOD";
    data.patientId    = "999";
    data.timestamp    = "2026/1/1";

    MedicalLabelPrintOptions options;
    options.model = MedicalLabelPrinterModel::GodexG500u;

    MedicalLabelPrinterModel resolved = MedicalLabelPrinterModel::Unknown;
    PrintJob job = renderMedicalLabel(data, options, &resolved);
    std::string zpl = job.asText();

    ASSERT(resolved == MedicalLabelPrinterModel::GodexG500u);
    ASSERT_EQ(job.format, "ezpl-gb2312");
    ASSERT(zpl.find("~S,ESG") != std::string::npos);
    ASSERT(zpl.find("^L") != std::string::npos);
    ASSERT(zpl.find("BQ,62,77,3,7,75,0,0,008085125") != std::string::npos);
    ASSERT(zpl.find("AC,18,10,1,2,0,0,22") != std::string::npos);
    ASSERT(zpl.find("AZ1,147,157,1,1,0,0,008085125") != std::string::npos);
    ASSERT(zpl.find("008085125") != std::string::npos);
}

ADD_TEST(ezpl_gb2312_backend_encodes_chinese_text) {
    LabelSettings settings = testSettings();
    LabelDocument doc(settings);
    doc.addText(5, 5, u8"中文测试", 24, 24, Font::Medium);

    const auto& profile = PrinterProfiles::godex_g500u();
    EzplGb2312Backend backend;
    PrintJob job = backend.render(doc, profile);

    const std::vector<uint8_t> gb2312ChineseTest = {
        0xD6, 0xD0, 0xCE, 0xC4, 0xB2, 0xE2, 0xCA, 0xD4
    };
    const auto found = std::search(job.data.begin(), job.data.end(),
                                   gb2312ChineseTest.begin(),
                                   gb2312ChineseTest.end());

    ASSERT_EQ(job.format, "ezpl-gb2312");
    ASSERT(job.debugText.find("AZ2,10,10,1,1,0,0") != std::string::npos);
    ASSERT(found != job.data.end());
}

// ---------------------------------------------------------------------------
// PrintJob tests
// ---------------------------------------------------------------------------
ADD_TEST(printjob_from_text) {
    PrintJob job = PrintJob::fromText("test", "hello");
    ASSERT_EQ(job.format, "test");
    ASSERT_EQ(job.asText(), "hello");
    ASSERT_EQ(job.data.size(), 5u);
}

ADD_TEST(printjob_binary_data_preserved) {
    PrintJob job;
    job.format = "raw";
    job.data   = {0x00, 0x01, 0x02, 0xFF};
    ASSERT_EQ(job.data.size(), 4u);
    ASSERT_EQ(job.data[0], 0x00);
    ASSERT_EQ(job.data[3], 0xFF);
}
