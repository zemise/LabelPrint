// Document model tests — verify LabelDocument, LabelSettings, elements
#include "test_utils.h"
#include "labelprint/document.h"
#include "labelprint/elements.h"
#include "labelprint/printer_profile.h"

using namespace labelprint;

ADD_TEST(document_holds_settings) {
    LabelSettings cfg;
    cfg.width    = 400;
    cfg.height   = 240;
    cfg.darkness = 25;
    LabelDocument doc(cfg);
    ASSERT_EQ(doc.settings().width, 400);
    ASSERT_EQ(doc.settings().height, 240);
    ASSERT_EQ(doc.settings().darkness, 25);
}

ADD_TEST(document_holds_text_elements) {
    LabelSettings cfg;
    LabelDocument doc(cfg);
    doc.addText(5, 5, "Hello", 30, 20, Font::Medium);
    ASSERT_EQ(doc.texts().size(), 1u);
    ASSERT_EQ(doc.texts()[0].x, 5);
    ASSERT_EQ(doc.texts()[0].text, "Hello");
    ASSERT_EQ(doc.texts()[0].height, 30);
    ASSERT_EQ(doc.texts()[0].fontName, Font::Medium);
}

ADD_TEST(document_holds_barcode_elements) {
    LabelSettings cfg;
    LabelDocument doc(cfg);
    doc.addBarcode(20, 72, "008085125", 75, 3, 3.0, false);
    ASSERT_EQ(doc.barcodes().size(), 1u);
    ASSERT_EQ(doc.barcodes()[0].data, "008085125");
    ASSERT_EQ(doc.barcodes()[0].height, 75);
    ASSERT_EQ(doc.barcodes()[0].printTextBelow, false);
}

ADD_TEST(document_holds_line_elements) {
    LabelSettings cfg;
    LabelDocument doc(cfg);
    doc.addLine(10, 20, 100, true, 2);
    ASSERT_EQ(doc.lines().size(), 1u);
    ASSERT_EQ(doc.lines()[0].horizontal, true);
    ASSERT_EQ(doc.lines()[0].length, 100);
    ASSERT_EQ(doc.lines()[0].thickness, 2);
}

ADD_TEST(document_holds_box_elements) {
    LabelSettings cfg;
    LabelDocument doc(cfg);
    doc.addBox(5, 5, 100, 50, 1);
    ASSERT_EQ(doc.boxes().size(), 1u);
    ASSERT_EQ(doc.boxes()[0].width, 100);
    ASSERT_EQ(doc.boxes()[0].height, 50);
    ASSERT_EQ(doc.boxes()[0].thickness, 1);
}

ADD_TEST(document_clear_removes_all_elements) {
    LabelSettings cfg;
    LabelDocument doc(cfg);
    doc.addText(0, 0, "A", 1, 1, Font::Default);
    doc.addBarcode(0, 0, "1", 1, 1, 3.0, true);
    doc.addLine(0, 0, 1, true, 1);
    doc.addBox(0, 0, 1, 1, 1);
    doc.clear();
    ASSERT_EQ(doc.texts().size(), 0u);
    ASSERT_EQ(doc.barcodes().size(), 0u);
    ASSERT_EQ(doc.lines().size(), 0u);
    ASSERT_EQ(doc.boxes().size(), 0u);
}

ADD_TEST(text_render_mode_defaults_to_auto) {
    TextElement t;
    ASSERT(t.renderMode == TextRenderMode::Auto);
}

ADD_TEST(barcode_type_is_code128) {
    BarcodeElement b;
    ASSERT(b.type == BarcodeType::Code128);
}

ADD_TEST(printer_profile_xp360b) {
    const auto& p = PrinterProfiles::xprinter_xp360b();
    ASSERT_EQ(p.dpi, 203);
    ASSERT(p.language == PrinterLanguage::TSPL);
    ASSERT_EQ(p.bitmapWhiteIsOne, true);
    ASSERT_EQ(p.nativeChinese, true);
    ASSERT_EQ(p.nativeChineseFont, "TSS24.BF2");
    ASSERT_EQ(p.nativeChineseCodepage, "54936");
}

ADD_TEST(printer_profile_zd888) {
    const auto& p = PrinterProfiles::zebra_zd888();
    ASSERT_EQ(p.dpi, 203);
    ASSERT(p.language == PrinterLanguage::ZPL);
    ASSERT_EQ(p.nativeBarcode, true);
    ASSERT_EQ(p.nativeChinese, true);
    ASSERT(p.textStrategy == TextStrategy::Native);
    ASSERT_EQ(p.nativeChineseFont, "E:CSONG.TTF");
}
