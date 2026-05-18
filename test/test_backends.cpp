// Backend output tests — verify ZPL and TSPL contain expected commands
#include "test_utils.h"
#include "labelprint/document.h"
#include "labelprint/printer_profile.h"
#include "labelprint/backend.h"
#include "labelprint/template.h"
#include "labelprint/zpl_backend.h"
#include "labelprint/tspl_backend.h"
#include "labelprint/tspl_bitmap_backend.h"
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
    doc.addBarcode(20, 72, "123456", 75, 3, false);
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
