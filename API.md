# API Reference · v1.0.0

## Architecture

```
MedicalLabelData          ← your business data
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

---

## Integration

### CMake `add_subdirectory` (recommended)

```cmake
# Your CMakeLists.txt
add_subdirectory(path/to/labelprint)
target_link_libraries(your_app PRIVATE labelprint)
```

The library target `labelprint` is a static library. It automatically sets up include paths (`include/`) and links `gdiplus.lib` on MSVC. C++17 is required.

### Pre-built release (no build needed)

Download `labelprint-vX.Y.Z-windows-x64.zip` from [GitHub Releases](https://github.com/zemise/labelprint/releases).

```
# Include
-Ilabelprint/include -Ilabelprint

# Link
labelprint.lib gdiplus.lib
```

### Manual build

```bash
git clone https://github.com/zemise/labelprint.git
cd labelprint
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Link: build/labelprint.lib + gdiplus.lib
# Include: -Ilabelprint/include -Ilabelprint
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
| C++ standard | C++17 or later |
| Compiler | MSVC (VS 2022+), Clang, GCC |
| Windows SDK | Required for `WindowsRawTransport` and `TsplBitmapBackend` |
| gdiplus.lib | Required (bundled with Windows SDK) |

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
doc.addBarcode(20, 72, "008085125", 75, 2, false);
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
    bool nativeChinese;           // has built-in CJK font
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
    std::string format;              // "zpl", "tspl", "tspl-bitmap"
    std::vector<uint8_t> data;       // raw bytes for the printer
    std::string debugText;           // human-readable form

    std::string asText() const;      // data as string (text formats only)
    static PrintJob fromText(const std::string& format, const std::string& text);
};
```

`asText()` works for `"zpl"` and `"tspl"` formats. For `"tspl-bitmap"`, use `debugText` to inspect the generated commands.

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
| `TsplBitmapBackend` | `labelprint/tspl_bitmap_backend.h` | `"tspl-bitmap"` | Yes (CJK → BITMAP) |

**Choosing a backend:**

```
ASCII text     → TsplBackend (fast, clean output)
Chinese text   → TsplBitmapBackend (GDI+ rendering, verified on XP-360B)
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
```

Produces a `LabelDocument` with 8 fields at verified XP-360B positions (50×30 mm label, 203 DPI). You can further modify the returned document before rendering.

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
TsplBitmapBackend backend;
PrintJob job = backend.render(doc, PrinterProfiles::xprinter_xp360b());
WindowsRawTransport().send(job, PrinterConnection{"Xprinter XP-360B #2"});
```

### Build a label without templates

```cpp
LabelDocument doc(LabelSettings{400, 240});
doc.addText(5, 5, "Patient: ABC", 30, 20, Font::Medium);
doc.addBarcode(20, 72, "1234567890", 80, 3);
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
    doc.addBarcode(20, 60, data.trackingNo, 80, 2);
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
LABELPRINT_VERSION_MINOR   // 0
LABELPRINT_VERSION_PATCH   // 0
LABELPRINT_VERSION_STRING  // "1.0.0"
```

The version also appears in CMake: `project(LabelPrint VERSION 1.0.0)`.

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
