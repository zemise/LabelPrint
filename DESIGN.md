# Label Printing Design Notes

## Goal

Generate and print a `50 mm x 30 mm` medical barcode label that includes:

- Sample number
- Test item
- Code 128 barcode
- Barcode text
- Patient name
- Specimen type
- Department
- Patient number
- Timestamp

## Final technical choice

The original project direction used `ZPL`, but the connected printer in this workspace is an `Xprinter XP-360B`, and the verified working path is now:

- `TSPL` for printer control and barcode output
- native TSPL Chinese text through `GB18030` + `TSS24.BF2`, with `TSPL BITMAP` retained as fallback

This split is intentional:

- `BARCODE` commands are crisp and native on the printer
- Chinese text through built-in printer fonts was not reliable
- Rendering Chinese text to bitmap in Windows gives stable output

## Working print architecture

### 1. Core C++ generator

The C++ layer in [zpl_label.h](C:/Mac/Home/Downloads/020%20barcode/zpl_label.h) and [zpl_label.cpp](C:/Mac/Home/Downloads/020%20barcode/zpl_label.cpp):

- Builds ZPL labels
- Exports a simple TSPL text form for ASCII-only scenarios
- Remains useful as the reusable label-building core

### 2. Windows production print path

The actual Chinese-capable print flow is:

1. [GenerateLabelTsplCn.ps1](C:/Mac/Home/Downloads/020%20barcode/GenerateLabelTsplCn.ps1)
   - Renders Chinese text with Windows fonts
   - Converts each Chinese text block into binary `BITMAP` payload bytes
   - Writes the final printer-ready `.prn` file

2. [RawTSPL.ps1](C:/Mac/Home/Downloads/020%20barcode/RawTSPL.ps1)
   - Reads the generated file as raw bytes
   - Sends it to the selected printer with WinSpool `RAW`

## Confirmed printer behavior

For this printer, Chinese `BITMAP` rendering was validated empirically.

The confirmed working rule is:

- Background bits should be encoded as `1`
- Text pixels should be encoded as `0`

This is why the final bitmap generator uses the equivalent of `BlackIsOne = $false`.

## Files kept on purpose

- [GenerateLabelTsplCn.ps1](C:/Mac/Home/Downloads/020%20barcode/GenerateLabelTsplCn.ps1): Production generator for Chinese-capable TSPL output
- [RawTSPL.ps1](C:/Mac/Home/Downloads/020%20barcode/RawTSPL.ps1): Production raw print sender
- [label_medical_cn.prn](C:/Mac/Home/Downloads/020%20barcode/label_medical_cn.prn): Latest working print payload
- [RawZPL.ps1](C:/Mac/Home/Downloads/020%20barcode/RawZPL.ps1): Historical reference from the earlier ZPL phase
- [label_medical.zpl](C:/Mac/Home/Downloads/020%20barcode/label_medical.zpl): Historical reference output
- [label_medical.tspl](C:/Mac/Home/Downloads/020%20barcode/label_medical.tspl): ASCII-only TSPL sample output

## Files removed during cleanup

The temporary bitmap polarity diagnosis artifacts were removed after the printer behavior was confirmed:

- `GenerateBitmapPolarityTest.ps1`
- `bitmap_polarity_test.prn`

## Phase 1 completed (2026-05-17)

Printer-independent document model extracted:

- `include/labelprint/elements.h` — `TextElement`, `BarcodeElement`, `LineElement`, `BoxElement`
- `include/labelprint/document.h` — `LabelDocument` container + `LabelSettings`
- `IPrinterBackend` + `ZplBackend` / `TsplBackend` map the document model to printer commands (Phase 3)

## Phase 2 completed (2026-05-17)

Printer profiles introduced:

- `include/labelprint/printer_profile.h` — `PrinterProfile` struct, `PrinterLanguage`/`TextStrategy` enums
- `src/labelprint/printer_profile.cpp` — Built-in `xprinter_xp360b()`, `zebra_zd888()`, and `godex_g500u()` factories
- DPI parameterized: `dotsToMm(dots, dpi)`, `dotsToTsplScale(dots, dpi)`
- `main.cpp` derives settings from profile; BITMAP padding bug fixed in `GenerateLabelTsplCn.ps1`

## Phase 3 completed (2026-05-17)

Backend interface established:

- `include/labelprint/backend.h` — `IPrinterBackend` + `PrintJob`
- `include/labelprint/zpl_backend.h` + `src/backends/zpl_backend.cpp` — `ZplBackend` implementation
- `include/labelprint/tspl_backend.h` + `src/backends/tspl_backend.cpp` — `TsplBackend` implementation
- `src/backends/detail.h` — Shared `mapFont`, `convertSettings`, `populate` helpers
- `main.cpp` updated to render via backends instead of bridge functions

## Phase 4 completed (2026-05-17)

Chinese bitmap rendering moved from PowerShell to C++:

- `include/labelprint/tspl_bitmap_backend.h` + `src/backends/tspl_bitmap_backend.cpp` — `TsplBitmapBackend` using Windows GDI+
- `include/labelprint/labelprint.h` — aggregate header (single include for consumers)
- `renderTextToBitmap()` — GDI+ font rasterization with polarity matching (white bg = 1)
- `containsNonAscii()` / `shouldUseBitmap()` — CJK detection and per-element strategy
- `zpl_label.h/.cpp` extended: `BitmapField` struct, `generateTsplBinary()`, binary-safe output
- `detail::populateTsplBitmap()` — populates ZplLabel with native + bitmap elements
- Text strategy enums now wired: `TextRenderMode`, `TextStrategy` drive actual rendering decisions
- `main.cpp` generates both ASCII and Chinese labels via backends

## Phase 5 completed (2026-05-17)

Template API established:

- `include/labelprint/template.h` — `MedicalLabelData` struct + `buildMedicalLabel()` declaration
- `src/templates/medical_label.cpp` — template implementation with verified label layout
- `main.cpp` simplified to populate `MedicalLabelData` and call `buildMedicalLabel()`
- Business code no longer needs to manage element positions or font selection

## Phase 6 completed (2026-05-18)

Transport layer implemented:

- `include/labelprint/transport.h` — `PrinterConnection`, `IPrintTransport` + 3 transports
- `src/transports/file_transport.cpp` — `FileTransport` (write to file, for debugging/export)
- `src/transports/windows_raw_transport.cpp` — `WindowsRawTransport` (WinSpool RAW, ported from PowerShell)
- `src/transports/tcp9100_transport.cpp` — `Tcp9100Transport` (network printer via TCP 9100)
- `main.cpp` uses `FileTransport` instead of inline `saveJob()` helper
- Render–transport separation complete: backends produce `PrintJob`, transports send it

## Phase 7 completed (2026-05-18)

Test infrastructure established:

- `test/test_utils.h` — Shared test macros (`ADD_TEST`, `ASSERT`, `ASSERT_EQ`) with inline registry
- `test/test_main.cpp` — Test runner entry point
- `test/test_document.cpp` — 11 document model & profile tests
- `test/test_backends.cpp` — 24 backend output & PrintJob tests
- `CMakeLists.txt` — `test_runner` CMake target
- All 35 tests pass

## Suggested next improvements

- `PreviewBackend` — label preview and snapshot tests
- More printer profiles
- Cross-platform bitmap rendering
