# API Reference · v1.2.6

## Architecture

```
MedicalLabelData          ← your business data
       │
       ▼
printMedicalLabel()       ← high-level API: model detection + render + send
       │
       ▼
buildMedicalLabel()       ← template (optional, or build LabelDocument by hand)
       │
       ▼
LabelDocument             ← printer-independent label description
       │
       ▼
Backend.render()          ← IPrinterBackend → PrintJob
       │
       ▼
Transport.send()          ← IPrintTransport → printer / file / TCP
```

Four layers, each with one job:

| Layer | Header | What it does |
|---|---|---|
| Data | `template.h` | Structured domain data (optional, use directly or write your own) |
| Document | `document.h`, `elements.h` | Printer-independent label description |
| Rendering | `backend.h` + backend headers | Converts document to printer commands |
| Sending | `transport.h` | Delivers the rendered job |

For application code that prints standard medical sample labels, prefer `printMedicalLabel()`. Use the lower layers only when you need custom documents, debug output files, or special transport behavior.

---

## Integration

### CMake `add_subdirectory` (recommended)

```cmake
# Your CMakeLists.txt
add_subdirectory(path/to/labelprint)
target_link_libraries(your_app PRIVATE labelprint)
```

The library target `labelprint` is a static library. It automatically sets up include paths (`include/`) and links the required Windows SDK libraries on Windows. C++17 is required.

### Pre-built release (no build needed)

Download one of the Windows x64 packages from [GitHub Releases](https://github.com/zemise/labelprint/releases):

| Package suffix | Use when |
|---|---|
| `windows-x64-vs2026` | Normal Windows x64 build |
| `windows-x64-vs2022-win7` | Windows 7 compatibility is required |

The Win7 package is fixed to `WINVER=0x0601`, `_WIN32_WINNT=0x0601`, and the static MSVC runtime.

Release layout:

```
include/labelprint/...
lib/labelprint.lib
cmake/LabelPrintConfig.cmake
cmake/LabelPrintTargets.cmake
README.md
API.md
```

Use it from CMake:

```cmake
find_package(LabelPrint 1.2 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE LabelPrint::labelprint)
```

Configure with the unzipped package root in `CMAKE_PREFIX_PATH`:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\path\to\labelprint-vX.Y.Z-windows-x64-vs2026
cmake --build build --config Release
```

The imported target provides the include path and Windows SDK link libraries.

### High-level medical label printing

```cpp
#include "labelprint/labelprint.h"

using namespace labelprint;

MedicalLabelData data;
data.sampleNo = "22";
data.testItem = u8"血常规";
data.barcodeValue = "008085125";
data.patientName = u8"廖明";
data.specimenType = u8"全血";
data.patientId = "202629988";
data.timestamp = "2026/5/15 9:24";

MedicalLabelPrintOptions options;
options.model = MedicalLabelPrinterModel::Auto;

MedicalLabelPrintResult result =
    printMedicalLabel("Xprinter XP-360B #2", data, options);
```

`MedicalLabelPrinterModel::Auto` reads Windows printer metadata such as driver, port, processor, comment, and display name. It automatically selects `Xprinter XP-360B` with `TsplGb18030Backend` or `Zebra ZD888` with `ZplBackend` when possible. If the printer cannot be identified, `fallbackModel` is used and defaults to `XprinterXp360b`.

Windows applications can call the `std::wstring` overload when the printer name may contain Chinese or other non-ANSI characters:

```cpp
printMedicalLabel(L"条码打印机", data, options);
```

Manual usage is also supported:

```
# Include
-Ilabelprint/include

# Link
labelprint.lib gdiplus.lib ws2_32.lib winspool.lib
```

### Manual build

```bash
git clone https://github.com/zemise/labelprint.git
cd labelprint
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix package
# Release-style package: package/include, package/lib, package/cmake
```

### Header

```cpp
#include "labelprint/labelprint.h"   // everything in one line
// or pick individual headers:
#include "labelprint/document.h"
#include "labelprint/zpl_backend.h"
// ...
```

### Build requirements

| Requirement | Notes |
|---|---|
| CMake | 3.15 or later |
| C++ standard | C++17 or later |
| Compiler | MSVC (VS 2022+), Clang, GCC |
| Windows SDK | Required for `WindowsRawTransport` and `TsplBitmapBackend` |
| Windows libraries | Manual link users must link `gdiplus.lib`, `ws2_32.lib`, and `winspool.lib` |

---

## Quick start

```cpp
#include "labelprint/labelprint.h"

using namespace labelprint;

int main() {
    // 1. Pick a printer profile
    const auto& profile = PrinterProfiles::xprinter_xp360b();

    // 2. Configure the label
    LabelSettings cfg;
    cfg.width  = 400;
    cfg.height = 240;
    cfg.darkness = profile.darkness;

    // 3. Fill your data
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = "CBC PANEL";
    data.barcodeValue = "008085125";
    data.patientName  = "LIAO MING";
    data.specimenType = "BLOOD";
    data.patientId    = "202629988";
    data.timestamp    = "2026/5/15 9:24";

    // 4. Build the document
    LabelDocument doc = buildMedicalLabel(data, cfg);

    // 5. Render to printer commands
    TsplBackend backend;
    PrintJob job = backend.render(doc, profile);

    // 6. Send
    FileTransport transport;
    transport.send(job, PrinterConnection{"label.tspl"});
}
```

---

## Layer 1 — Elements (`elements.h`)

### TextElement

```cpp
struct TextElement {
    int x = 0;                    // horizontal position (dots)
    int y = 0;                    // vertical position (dots)
    std::string text;             // UTF-8 text content
    int height = 30;              // character height (dots)
    int width  = 20;              // character width (dots)
    std::string fontName = Font::Default;
    TextRenderMode renderMode = TextRenderMode::Auto;
};
```

**`renderMode` values:**

| Value | Behavior |
|---|---|
| `TextRenderMode::Auto` | Backend decides: native for ASCII, bitmap for CJK |
| `TextRenderMode::Native` | Force printer built-in font |
| `TextRenderMode::Bitmap` | Force bitmap rendering (GDI+ on Windows) |

**`fontName` values** (in `Font::` namespace):

| Constant | String value | Typical use |
|---|---|---|
| `Font::Default` | `"default"` | Body text |
| `Font::Small` | `"small"` | Compact labels |
| `Font::Medium` | `"medium"` | Sub-headings |
| `Font::Large` | `"large"` | Titles |
| `Font::Bold` | `"bold"` | Emphasis |

### BarcodeElement

```cpp
struct BarcodeElement {
    int x = 0;
    int y = 0;
    std::string data;                    // barcode payload
    BarcodeType type = BarcodeType::Code128;
    int height = 100;                    // bar height (dots)
    int narrowWidth = 2;                 // narrow bar width (dots)
    double wideRatio = 3.0;              // wide:narrow width ratio
    bool printTextBelow = true;          // show human-readable text under bars
};
```

**`BarcodeType` values:** `Code128`, `Code39`, `EAN13`, `QRCode`

### LineElement

```cpp
struct LineElement {
    int x = 0;
    int y = 0;
    int length = 0;
    bool horizontal = true;
    int thickness = 1;           // line thickness (dots)
};
```

### BoxElement

```cpp
struct BoxElement {
    int x = 0;
    int y = 0;
    int width  = 0;
    int height = 0;
    int thickness = 1;           // border thickness (dots)
};
```

---

## Layer 1 — Document (`document.h`)

### LabelSettings

```cpp
struct LabelSettings {
    int width      = 400;    // label width (dots)
    int height     = 600;    // label height (dots)
    int homeX      = 20;     // origin X offset (dots)
    int homeY      = 20;     // origin Y offset (dots)
    int darkness   = 25;     // print darkness (0–30)
    int printSpeed = 4;      // print speed (inches/sec)
    int quantity   = 1;      // number of copies
    bool tearOff   = true;   // tear-off mode
};
```

Use `profile.darkness` and `profile.speed` to match the printer's recommended range.

### LabelDocument

The top-level container. Holds one `LabelSettings` and vectors of elements.

```cpp
class LabelDocument {
public:
    LabelDocument();
    explicit LabelDocument(const LabelSettings& s);

    // Element access (read-only)
    const std::vector<TextElement>&    texts()    const;
    const std::vector<BarcodeElement>& barcodes() const;
    const std::vector<LineElement>&    lines()    const;
    const std::vector<BoxElement>&     boxes()    const;

    // Settings
    const LabelSettings& settings() const;
    void setSettings(const LabelSettings& s);

    // Add elements (convenience overloads)
    TextElement& addText(int x, int y, const std::string& text,
                         int height = 30, int width = 20,
                         const std::string& font = Font::Default);
    TextElement& addText(const TextElement& t);

    BarcodeElement& addBarcode(int x, int y, const std::string& data,
                               int barHeight = 100, int narrowWidth = 2,
                               double wideRatio = 3.0,
                               bool printText = true);
    BarcodeElement& addBarcode(const BarcodeElement& b);

    LineElement& addLine(int x, int y, int length,
                         bool horizontal = true, int thickness = 1);
    LineElement& addLine(const LineElement& l);

    BoxElement& addBox(int x, int y, int w, int h, int thickness = 1);
    BoxElement& addBox(const BoxElement& b);

    void clear();  // remove all elements, keep settings
};
```

All `add*()` methods return a reference to the stored element so you can tweak it:

```cpp
auto& t = doc.addText(10, 20, "Hello");
t.renderMode = TextRenderMode::Bitmap;   // force bitmap for this one
t.fontName   = Font::Large;
```

**Building a label without a template:**

```cpp
LabelDocument doc(LabelSettings{400, 240});
doc.addText(5, 5, "Sample: 22", 30, 20, Font::Medium);
doc.addBarcode(20, 72, "008085125", 75, 2, 3.0, false);
doc.addLine(10, 160, 380, true, 1);
doc.addBox(0, 0, 400, 240, 2);
```

---

## Layer 2 — Printer Profile (`printer_profile.h`)

### PrinterProfile

Describes a printer model's capabilities and defaults.

```cpp
struct PrinterProfile {
    std::string name;
    PrinterLanguage language;     // ZPL or TSPL
    int dpi;                      // dots per inch

    int maxWidth, maxHeight;      // printable area (dots)

    bool nativeBarcode;           // supports built-in BARCODE commands
    bool nativeChinese;           // has available CJK font support
    std::string nativeChineseFont; // e.g. E:CSONG.TTF for Zebra ZPL
    bool bitmapGraphics;          // supports BITMAP / ~DG commands

    TextStrategy textStrategy;    // recommended approach for text

    int gapMm, gapOffsetMm;       // label gap
    int darkness;                 // recommended darkness
    int speed;                    // recommended speed
    int homeX, homeY;             // origin offset (dots)

    bool bitmapWhiteIsOne;        // BITMAP polarity (see below)
};
```

**`bitmapWhiteIsOne`** — Determines how bitmap bytes encode white background.
- `true` on XP-360B: white pixel = bit 1, black pixel = bit 0
- `false` on most others: black pixel = bit 1, white pixel = bit 0

### Built-in profiles

```cpp
const PrinterProfile& PrinterProfiles::xprinter_xp360b();  // 203 DPI TSPL
const PrinterProfile& PrinterProfiles::zebra_zd888();      // 203 DPI ZPL
```

Always use these factories. They return `const&` to a static instance — safe to hold by reference.

---

## Layer 3 — Backends (`backend.h` + backend-specific headers)

### PrintJob

Rendered output from a backend.

```cpp
struct PrintJob {
    std::string format;              // "zpl", "tspl", "tspl-gb18030", ...
    std::vector<uint8_t> data;       // raw bytes for the printer
    std::string debugText;           // human-readable form

    std::string asText() const;      // data as string (text formats only)
    static PrintJob fromText(const std::string& format, const std::string& text);
};
```

`asText()` works for `"zpl"` and ASCII `"tspl"` formats. For binary-capable formats such as `"tspl-gb18030"` and `"tspl-bitmap"`, use `debugText` to inspect the generated commands.

### IPrinterBackend

```cpp
class IPrinterBackend {
public:
    virtual ~IPrinterBackend() = default;
    virtual PrintJob render(const LabelDocument& doc,
                            const PrinterProfile& profile) = 0;
};
```

### Concrete backends

| Backend | Header | Format | CJK support |
|---|---|---|---|
| `ZplBackend` | `labelprint/zpl_backend.h` | `"zpl"` | No (CI28 + built-in font) |
| `TsplBackend` | `labelprint/tspl_backend.h` | `"tspl"` | ASCII only |
| `TsplGb18030Backend` | `labelprint/tspl_gb18030_backend.h` | `"tspl-gb18030"` | Yes (`CODEPAGE 54936` + `TSS24.BF2`) |
| `TsplBitmapBackend` | `labelprint/tspl_bitmap_backend.h` | `"tspl-bitmap"` | Yes (CJK → BITMAP) |

**Choosing a backend:**

```
ASCII text     → TsplBackend (fast, clean output)
Chinese text   -> TsplGb18030Backend for XP-360B, native ZPL font for verified ZD888 profiles
Fallback CJK   -> TsplBitmapBackend when native TSPL Chinese fonts are unavailable
Zebra printer  → ZplBackend (ZPL output)
```

**CJK text strategy** (applies to `TsplBitmapBackend`):

The backend decides per `TextElement` whether to use native TEXT or BITMAP:
1. If `renderMode` is `Native` → always native TEXT
2. If `renderMode` is `Bitmap` → always BITMAP
3. If `renderMode` is `Auto` → auto-detect: ASCII → native, CJK → BITMAP

---

## Layer 4 — Transports (`transport.h`)

### PrinterConnection

```cpp
struct PrinterConnection {
    std::string name;        // printer share name or IP address, or file path
    int port = 9100;         // TCP port (for Tcp9100Transport)
};
```

### IPrintTransport

```cpp
class IPrintTransport {
public:
    virtual ~IPrintTransport() = default;
    virtual void send(const PrintJob& job,
                      const PrinterConnection& conn) = 0;
};
```

### FileTransport

Writes `job.data` as raw bytes to a file.

```cpp
FileTransport ft;
ft.send(job, PrinterConnection{"output.prn"});
```

### WindowsRawTransport

Sends RAW print job via WinSpool API (`OpenPrinterA` → `WritePrinter`).

```cpp
WindowsRawTransport raw;
raw.send(job, PrinterConnection{"Xprinter XP-360B #2"});
```

`conn.name` is the Windows printer share name (exactly as shown in Control Panel).

### Tcp9100Transport

Sends job to a network printer via TCP port 9100.

```cpp
Tcp9100Transport tcp;
tcp.send(job, PrinterConnection{"192.168.1.100", 9100});
```

---

## Layer 1b — Templates (`template.h`)

### MedicalLabelData

Domain struct for a medical sample label. All fields are UTF-8 `std::string`.

```cpp
struct MedicalLabelData {
    std::string sampleNo;        // e.g. "22"
    std::string testItem;        // e.g. u8"血常规（迈瑞流水线）"
    std::string barcodeValue;    // e.g. "008085125"
    std::string patientName;     // e.g. u8"廖明"
    std::string specimenType;    // e.g. u8"全血"
    std::string patientId;       // e.g. "202629988"
    std::string timestamp;       // e.g. "2026/5/15 9:24"
    std::string department;      // e.g. u8"心血管内科二区" (optional)
};
```

### buildMedicalLabel()

```cpp
LabelDocument buildMedicalLabel(const MedicalLabelData& data,
                                const LabelSettings& cfg);

LabelDocument buildMedicalLabel(const MedicalLabelData& data,
                                const MedicalLabelLayout& layout);
```

Produces a `LabelDocument` with 8 fields at verified XP-360B positions (50×30 mm label, 203 DPI). You can further modify the returned document before rendering.

### MedicalLabelLayout

Use `MedicalLabelLayout` when label geometry or field positions need to vary by printer, media, or hospital template.

```cpp
struct MedicalLabelLayout {
    LabelSettings settings;              // width, height, homeX, homeY, etc.
    int rowGap = 30;                     // row spacing hint for callers

    MedicalLabelTextLayout sampleNo;
    MedicalLabelTextLayout testItem;
    MedicalLabelBarcodeLayout barcode;
    MedicalLabelTextLayout barcodeText;
    MedicalLabelTextLayout patientName;
    MedicalLabelTextLayout specimenType;
    MedicalLabelTextLayout department;
    MedicalLabelTextLayout patientId;
    MedicalLabelTextLayout timestamp;
};
```

**`MedicalLabelTextLayout`** — controls position, size, font, and optional wrapping:

```cpp
struct MedicalLabelTextLayout {
    MedicalLabelPoint pos;
    int height = 30;          // character height (dots)
    int width = 20;           // character width (dots)
    std::string font = Font::Default;
    int maxWidth = 0;         // max width before wrapping (dots, 0 = no wrap)
    int lineGap = 2;          // vertical gap between wrapped lines (dots)
    int maxLines = 1;         // max number of wrapped lines
    MedicalLabelTextAlign align = MedicalLabelTextAlign::Left;
};
```

**`MedicalLabelBarcodeLayout`** — controls barcode position, size, and appearance:

```cpp
struct MedicalLabelBarcodeLayout {
    MedicalLabelPoint pos;
    int height = 75;            // bar height (dots)
    int narrowWidth = 2;        // narrow bar width (dots)
    double wideRatio = 2.0;     // wide:narrow bar ratio
    bool printTextBelow = false;
};
```

Each text field controls position, character size, and font:

```cpp
MedicalLabelLayout layout;
layout.settings.width = 400;
layout.settings.height = 240;
layout.settings.homeX = 5;
layout.settings.homeY = 5;

layout.sampleNo = {{5, 5}, 28, 16, Font::Medium};
layout.testItem = {{88, 8}, 18, 13, Font::Medium, 290, 2, 2};
layout.barcode = {{66, 72}, 75, 2, 3.0, false};
layout.barcodeText = {{0, 152}, 18, 13, Font::Medium, 400, 2, 1, MedicalLabelTextAlign::Center};

LabelDocument doc = buildMedicalLabel(data, layout);
```

The default `MedicalLabelLayout` targets 50×30 mm labels at 203 DPI (400×240 dots).
The built-in XP-360B auto-print path widens the barcode to `narrowWidth = 3`, `wideRatio = 2.6`, shifts the barcode area slightly left, centers `barcodeText`, and uses compact lower-row text. Zebra ZD888 keeps the shared default layout unless you pass a custom layout.

### Stable API surface

These names are kept stable for consumers:

- `MedicalLabelData`
- `LabelSettings`
- `buildMedicalLabel`
- `MedicalLabelPrintOptions`
- `printMedicalLabel`
- `TsplGb18030Backend`
- `TsplBitmapBackend`
- `WindowsRawTransport`

---

## Common recipes

### Generate a ZPL file

```cpp
#include "labelprint/labelprint.h"

LabelDocument doc = buildMedicalLabel(data, cfg);
ZplBackend backend;
PrintJob job = backend.render(doc, PrinterProfiles::zebra_zd888());
FileTransport().send(job, PrinterConnection{"label.zpl"});
```

### Print directly to XP-360B (Chinese)

```cpp
#include "labelprint/labelprint.h"

LabelDocument doc = buildMedicalLabel(data, cfg);
TsplGb18030Backend backend;
PrintJob job = backend.render(doc, PrinterProfiles::xprinter_xp360b());
WindowsRawTransport().send(job, PrinterConnection{"Xprinter XP-360B #2"});
```

### Build a label without templates

```cpp
LabelDocument doc(LabelSettings{400, 240});
doc.addText(5, 5, "Patient: ABC", 30, 20, Font::Medium);
doc.addBarcode(20, 72, "1234567890", 80, 3, 3.0);
doc.addLine(5, 160, 390, true, 1);
doc.addBox(0, 0, 400, 240, 2);

TsplBackend backend;
PrintJob job = backend.render(doc, PrinterProfiles::xprinter_xp360b());
FileTransport().send(job, PrinterConnection{"custom_label.tspl"});
```

### Force specific text to bitmap

```cpp
auto& t = doc.addText(10, 10, u8"中文", 30, 20);
t.renderMode = TextRenderMode::Bitmap;  // force GDI+ rendering
```

### Write your own template

```cpp
LabelDocument buildShippingLabel(const ShippingData& data, const LabelSettings& cfg) {
    LabelDocument doc(cfg);
    doc.addText(10, 10, data.trackingNo, 35, 22, Font::Bold);
    doc.addBarcode(20, 60, data.trackingNo, 80, 2, 3.0);
    doc.addText(10, 150, data.address, 28, 18, Font::Small);
    return doc;
}
```

### Chain multiple transports (save + print)

```cpp
PrintJob job = backend.render(doc, profile);
FileTransport().send(job, PrinterConnection{"debug_output.prn"});
WindowsRawTransport().send(job, PrinterConnection{"Xprinter XP-360B #2"});
```

---

## Version

```cpp
#include "labelprint/labelprint.h"

// Compile-time version macros
LABELPRINT_VERSION_MAJOR   // 1
LABELPRINT_VERSION_MINOR   // 2
LABELPRINT_VERSION_PATCH   // 6
LABELPRINT_VERSION_STRING  // "1.2.6"
```

The version also appears in CMake: `project(LabelPrint VERSION 1.2.6)`.

---

## Include path setup

Add one directory to your compiler include paths:

```
-I<lib>/include
```

Where `<lib>` is the root of this project. Include the library with a single header:

```cpp
#include "labelprint/labelprint.h"
```

Or include individual headers as needed (e.g. `#include "labelprint/document.h"`).

## Build requirements

- C++17
- Windows: MSVC, link `gdiplus.lib` for `TsplBitmapBackend`
- CMake: see `CMakeLists.txt` for reference target setup

## More info

- [ARCHITECTURE.md](ARCHITECTURE.md) — design rationale and roadmap
- [DESIGN.md](DESIGN.md) — printer verification notes
- [README.md](README.md) — project overview and workflow
