# LabelPrint · v1.2.4

![CI](https://github.com/zemise/labelprint/actions/workflows/ci.yml/badge.svg)

C++ label printing library — printer-independent document model with ZPL/TSPL backends.

## Current status

- The verified printer path is `TSPL`.
- Barcode and ASCII fields print through native `TSPL` commands.
- Chinese fields print through native XP-360B `GB18030` + `TSS24.BF2` TSPL, with bitmap rendering retained as fallback.
- The confirmed working printer in this workspace is `Xprinter XP-360B #2`.
- The connected `Zebra ZD888` path has been verified with native `ZPL` output, RAW WinSpool sending, and native Chinese text via `E:CSONG.TTF`.
- **Phase 1 & 2 complete:** Document model + printer profiles.
- **Phase 3 complete:** Backend interface (`IPrinterBackend` + `ZplBackend` + `TsplBackend`).
- **Phase 4 complete:** `TsplBitmapBackend` with GDI+ Chinese text rendering, plus XP-360B native GB18030 TSPL output.
- **Phase 5 complete:** Template API (`MedicalLabelData` + `buildMedicalLabel`).
- **Phase 6 complete:** Transport layer (`FileTransport`, `WindowsRawTransport`, `Tcp9100Transport`).
- **Phase 7 complete:** Test infrastructure (`test_runner.exe`), 31 tests covering document model + backend output.
- **Phase 8 complete:** High-level medical label print API (`printMedicalLabel`) that resolves printer model and backend internally.

## Project structure

```
include/labelprint/
  labelprint.h          — Aggregate header (single include for all public API)
  elements.h            — Text, Barcode, Line, Box element types
  document.h            — LabelDocument + LabelSettings
  printer_profile.h     — PrinterProfile + built-in profiles (Phase 2)
  backend.h             — IPrinterBackend + PrintJob (Phase 3)
  zpl_backend.h         — ZplBackend (Phase 3)
  tspl_backend.h        — TsplBackend (Phase 3)
  tspl_gb18030_backend.h — TsplGb18030Backend (XP-360B native Chinese)
  tspl_bitmap_backend.h — TsplBitmapBackend (Phase 4)
  transport.h           — IPrintTransport + all transports (Phase 6)
  template.h            — MedicalLabelData + buildMedicalLabel (Phase 5)
  medical_label_print.h — printMedicalLabel high-level API (Phase 8)
src/labelprint/
  printer_profile.cpp   — XP-360B and ZD888 profile factories
  medical_label_print.cpp — printer detection, backend selection, RAW print
src/templates/
  medical_label.cpp     — buildMedicalLabel() template implementation
src/transports/
  file_transport.cpp     — FileTransport: write PrintJob to file
  windows_raw_transport.cpp — WindowsRawTransport: WinSpool RAW print
  tcp9100_transport.cpp — Tcp9100Transport: TCP port 9100 network printer
src/backends/
  detail.h              — Shared font/settings conversion + populateTsplBitmap
  zpl_backend.cpp       — ZplBackend implementation
  tspl_backend.cpp      — TsplBackend implementation
  tspl_gb18030_backend.cpp — TsplGb18030Backend implementation
  tspl_bitmap_backend.cpp — TsplBitmapBackend implementation
test/
  smoke.ps1             — Full smoke test (build + both prints)
  quick_print.ps1       — ASCII label quick test
  quick_cn.ps1          — Chinese label quick test
  test_utils.h          — Shared test macros (ADD_TEST, ASSERT, ASSERT_EQ)
  test_main.cpp         — Test runner entry point
  test_document.cpp     — Document model & profile tests (10 tests)
  test_backends.cpp     — Backend output & PrintJob tests (20 tests)
zpl_label.h/cpp         — ZPL/TSPL command builder (internal engine)
main.cpp                — Sample using LabelDocument + PrinterProfile + Backends
CMakeLists.txt          — CMake build config
```

## Creating a release

```bash
git tag v1.0.1
git push origin v1.0.1
```

The [Release](.github/workflows/release.yml) workflow automatically:
- Builds `labelprint.lib` (Release, Windows x64)
- Runs the full test suite
- Packages headers, library, CMake config files, and docs into a `.zip`
- Creates a GitHub Release with the artifact

Release zip layout:

```
include/labelprint/...
lib/labelprint.lib
cmake/LabelPrintConfig.cmake
cmake/LabelPrintConfigVersion.cmake
cmake/LabelPrintTargets.cmake
cmake/LabelPrintTargets-release.cmake
README.md
API.md
```

Windows release variants:

| Package suffix | Toolchain | Compatibility |
|---|---|---|
| `windows-x64-vs2026` | Windows latest runner / current Visual Studio toolchain | Normal Windows x64 package |
| `windows-x64-vs2022-win7` | Windows 2022 runner / Visual Studio 2022 | Windows 7 compatible package |

The Win7 compatible package is always built with:

```cmake
WINVER=0x0601
_WIN32_WINNT=0x0601
CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
```

---

## Using the library in other projects

### Method 1: CMake package from release zip (recommended for consumers)

Unzip the release package, then point `CMAKE_PREFIX_PATH` at the package root:

```cmake
cmake_minimum_required(VERSION 3.15)
project(your-app LANGUAGES CXX)

find_package(LabelPrint 1.2 CONFIG REQUIRED)

add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE LabelPrint::labelprint)
```

Configure:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\path\to\labelprint-v1.2.4-windows-x64-vs2026
cmake --build build --config Release
```

### Method 2: CMake `add_subdirectory`

Copy this repository into your project tree, then:

```cmake
# Your project's CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(your-app LANGUAGES CXX)

add_subdirectory(libs/labelprint)

add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE labelprint)
```

Your C++ code:

```cpp
#include "labelprint/labelprint.h"
using namespace labelprint;

int main() {
    MedicalLabelData data;
    data.sampleNo     = "22";
    data.testItem     = u8"血常规";
    data.barcodeValue = "008085125";
    data.patientName  = u8"廖明";
    data.specimenType = u8"全血";
    data.patientId    = "202629988";
    data.timestamp    = "2026/5/15 9:24";

    MedicalLabelPrintOptions options;
    options.model = MedicalLabelPrinterModel::Auto;
    printMedicalLabel("Xprinter XP-360B #2", data, options);
}
```

`printMedicalLabel()` detects the Windows printer driver/name metadata and chooses the built-in XP-360B native GB18030 TSPL path or Zebra ZD888 ZPL path. If detection is inconclusive, it falls back to `options.fallbackModel`, which defaults to XP-360B for backward compatibility.
On Windows, a `std::wstring` overload is also available so applications can pass non-ANSI printer names without converting them through the local code page.

For printer- or template-specific layouts, pass `MedicalLabelLayout` instead of raw `LabelSettings`:

```cpp
MedicalLabelLayout layout;
layout.settings.width = 400;
layout.settings.height = 240;
layout.settings.homeX = 5;
layout.settings.homeY = 5;
layout.sampleNo = {{5, 5}, 28, 16, Font::Medium};
layout.barcode = {{66, 72}, 75, 2, 3.0, false};
layout.barcodeText = {{0, 152}, 18, 13, Font::Medium, 400, 2, 1, MedicalLabelTextAlign::Center};
layout.patientName = {{5, 175}, 28, 22, Font::Medium};

LabelDocument doc = buildMedicalLabel(data, layout);
```

The built-in XP-360B auto-print layout uses a wider barcode (`narrowWidth = 3`, `wideRatio = 2.6`) and shifts the barcode area slightly left while centering the human-readable barcode value. Zebra keeps the shared default layout unless a custom layout is supplied.

Stable public API:

- `MedicalLabelData`
- `LabelSettings`
- `buildMedicalLabel`
- `MedicalLabelPrintOptions`
- `printMedicalLabel`
- `TsplGb18030Backend`
- `TsplBitmapBackend`
- `WindowsRawTransport`

### Zebra ZD888 validation

Use the Zebra quick validation script:

```powershell
.\test\quick_zebra.ps1 -PrinterName "ZDesigner ZD888-203dpi ZPL"
```

This validates the full Zebra path:

- `PrinterProfiles::zebra_zd888()`
- `ZplBackend`
- native Chinese font rendering through `E:CSONG.TTF`
- generated `label_medical.zpl`
- RAW WinSpool sending through `RawZPL.ps1`

### Method 3: Manual include + link

Build the static library first:

```powershell
cd labelprint
cmake -S . -B out/build/x64-Release -G Ninja
cmake --build out/build/x64-Release
```

Then in your project:

```
Compiler flags:  /std:c++17 -Ilabelprint/include
Linker flags:    labelprint.lib gdiplus.lib ws2_32.lib winspool.lib
```

### Build requirements

- C++17
- CMake 3.15 or later
- Windows: MSVC (VS 2022 or later)
- Manual link users must link `gdiplus.lib`, `ws2_32.lib`, and `winspool.lib`

---

## Recommended workflow (this project itself)

### 1. Build

```powershell
# From Visual Studio Developer PowerShell:
cmake -S . -B out/build/x64-Debug -G Ninja
cmake --build out/build/x64-Debug
```

### 2. Generate labels

```bash
./out/build/x64-Debug/labelprint_demo
```

Produces `label_medical.zpl`, `label_medical.tspl`, and `label_medical_cn_new.prn`.
(Chinese label via `TsplGb18030Backend`; `TsplBitmapBackend` remains available as fallback.)

### 3. Send to the printer

```powershell
powershell -ExecutionPolicy Bypass -File .\RawTSPL.ps1 -PrinterName "Xprinter XP-360B #2" -TsplFile "label_medical_cn_new.prn"
```

## Important files

| File | Purpose |
|---|---|
| `include/labelprint/labelprint.h` | Aggregate header — one `#include` for all public API |
| `include/labelprint/elements.h` | Printer-independent element types |
| `include/labelprint/document.h` | `LabelDocument` container + `LabelSettings` |
| `include/labelprint/printer_profile.h` | `PrinterProfile` + built-in profiles |
| `src/labelprint/printer_profile.cpp` | XP-360B and ZD888 profile factories |
| `zpl_label.h` / `zpl_label.cpp` | ZPL/TSPL command builder (internal engine) |
| `include/labelprint/zpl_backend.h` / `src/backends/zpl_backend.cpp` | ZplBackend — renders LabelDocument to ZPL |
| `include/labelprint/tspl_backend.h` / `src/backends/tspl_backend.cpp` | TsplBackend — renders LabelDocument to TSPL |
| `include/labelprint/tspl_gb18030_backend.h` / `src/backends/tspl_gb18030_backend.cpp` | TsplGb18030Backend — XP-360B native Chinese via `CODEPAGE 54936` + `TSS24.BF2` |
| `include/labelprint/tspl_bitmap_backend.h` / `src/backends/tspl_bitmap_backend.cpp` | TsplBitmapBackend — CJK→BITMAP (Phase 4) |
| `src/backends/detail.h` | Shared font/settings conversion helpers |
| `main.cpp` | C++ sample using backends API |
| `GenerateLabelTsplCn.ps1` | Chinese bitmap TSPL generator (superseded, kept as reference) |
| `test/test_utils.h` | Shared test macros and registry |
| `test/test_main.cpp` | Test runner (31 tests, run with `test_runner.exe`) |
| `RawTSPL.ps1` | RAW print sender via WinSpool |
| `RawZPL.ps1` | RAW ZPL sender (reference) |
| `preview_render.py` | Local PIL-based label preview |

## Architecture roadmap

See [ARCHITECTURE.md](ARCHITECTURE.md) for the full 7-phase plan.

- [x] Phase 1: Extract printer-independent document model
- [x] Phase 2: Printer profiles
- [x] Phase 3: Backend interface
- [x] Phase 4: Formalize XP-360B Chinese path in C++
- [x] Phase 5: Template API
- [x] Phase 6: Transport implementations
- [x] Phase 7: Test infrastructure

## Running tests

```bash
# Build and run C++ unit tests (31 tests)
cmake --build out/build/x64-Debug --target test_runner
./out/build/x64-Debug/test_runner.exe
```

## Quick test scripts

During development, use the test scripts under `test/`:

```powershell
# Full smoke test: build → ASCII print → Chinese print
.\test\smoke.ps1

# Chinese label only
.\test\quick_cn.ps1

# ASCII label only (build + generate + print)
.\test\quick_print.ps1
```

## Notes

- The `ZplLabel` class in `zpl_label.h/cpp` is the internal ZPL/TSPL command builder, used by backends via `src/backends/detail.h`.
- `RawZPL.ps1` and the older ZPL artifacts remain only as reference for the earlier ZPL-based approach.
- The printer bitmap polarity was verified experimentally: white background bits = 1.
