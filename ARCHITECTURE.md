# 标签打印库架构设计

## 目的

本文档定义了当前项目演进为可复用 C++ 标签打印库的目标架构。该库应具备以下能力：

- 可嵌入其他 C++ 项目
- 支持方便的标签排版与模板化组合
- 通过打印机配置档案支持多个品牌和型号
- 在需要时同时支持原生条码打印与中文位图文本
- 将标签描述、布局、渲染与发送解耦

这是一份面向未来实现的设计文档。它描述的是目标方向，而不仅仅是当前代码状态。

## 设计目标

### 主要目标

- 提供与打印机无关的标签描述模型
- 让标签排版比手写 ZPL 或 TSPL 更方便
- 通过统一 API 支持多种打印机后端
- 通过打印机档案配置不同型号的差异
- 在可能的情况下保留原生条码输出的清晰度
- 为不稳定支持中文的打印机提供可靠的 CJK 输出方案

### 次要目标

- 在没有真实打印机的情况下也能生成预览
- 让“生成输出”和“发送到打印机”彼此独立
- 让后续测试与回归对比变得可行
- 同时支持底层 API 和高层模板 API

## 高层架构

系统分为四层：

1. 文档层
2. 布局层
3. 后端层
4. 传输层

### 1. 文档层

文档层负责描述“标签上要出现什么”。

它不关心：

- ZPL
- TSPL
- RAW Spool API
- 具体打印机品牌

它只关心逻辑标签元素，例如：

- 文本
- 条码
- 直线
- 矩形框
- 图像

### 2. 布局层

布局层负责决定“元素放在哪里”。

它应支持：

- 绝对定位
- 简单行列布局
- 基于锚点的对齐
- 基于模板的字段放置
- 未来的文本自动适配能力

### 3. 后端层

后端层负责把统一的标签文档转换成特定打印机格式。

例如：

- ZPL 后端
- TSPL 后端
- 中文位图 TSPL 后端
- 预览后端

### 4. 传输层

传输层负责把已经渲染好的结果发送到目标位置。

例如：

- Windows RAW 打印发送
- TCP 9100 网络发送
- 文件输出

## 核心概念

### LabelDocument

`LabelDocument` 是顶层模型，表示一个完整的标签任务。

建议职责：

- 保存页面尺寸
- 保存全局打印设置
- 持有标签元素列表

### LabelSettings

`LabelSettings` 用于保存与打印机无关的标签设置，例如：

- 宽度
- 高度
- 边距或原点偏移
- 打印浓度
- 打印速度
- 打印份数

当前项目中的 `LabelSettings` 结构体可以向这个方向演进。

### LabelElement

`LabelElement` 是可打印元素的抽象基概念。

预期的具体元素类型：

- `TextElement`
- `BarcodeElement`
- `LineElement`
- `BoxElement`
- `ImageElement`

每个元素应只携带逻辑属性，而不是原始打印机命令。

### TextElement

预期字段：

- 文本内容
- 位置或布局约束
- 字体规格
- 对齐方式
- 渲染模式偏好

渲染模式建议支持：

- `Native`
- `Bitmap`
- `Auto`

### BarcodeElement

预期字段：

- 数据内容
- 条码类型
- 窄线宽度
- 高度
- 是否打印人可读文本

条码类型建议定义为逻辑枚举，例如：

- `Code128`
- `Code39`
- `EAN13`
- `QRCode`

具体后端再把这些逻辑类型映射为实际打印语言命令。

### ImageElement

这个元素以后会很重要，适用于：

- 中文位图文本回退
- Logo
- 任意光栅内容

## 布局系统

第一阶段实现应保持简单。

### 第一阶段布局模型

支持：

- 以 dots 为单位的绝对坐标
- 固定尺寸
- 显式字号

这与当前项目最匹配，也能降低实现风险。

### 第二阶段布局模型

增加轻量布局辅助：

- 横向排列
- 纵向排列
- 锚点定位
- 简单文本测量接口

### 第三阶段模板模型

增加可复用模板，例如：

- 医疗样本标签
- 面单
- 资产标签

模板代码应从结构化业务数据构建 `LabelDocument`。

## 打印机档案系统

打印机差异不能散落在各处代码里。

应引入 `PrinterProfile` 抽象，字段建议包括：

- 档案名称
- 打印语言
- DPI
- 最大宽度
- 最大高度
- 是否支持原生条码
- 是否支持原生中文
- 是否支持位图图形
- 推荐文本策略
- 默认 gap 参数
- 打印浓度范围
- 打印速度范围

### 内置档案示例

- `Xprinter XP-360B`
- `Zebra ZD888`

### 档案职责

档案负责定义能力和行为，不负责业务内容。

例如：

- 中文是否应走位图回退
- 哪些条码类型可原生支持
- 应优先使用哪个后端

## 后端设计

应引入统一渲染接口。

### 建议接口

```cpp
class IPrinterBackend {
public:
    virtual ~IPrinterBackend() = default;
    virtual PrintJob render(const LabelDocument& doc, const PrinterProfile& profile) = 0;
};
```

### PrintJob

`PrintJob` 表示后端渲染后的结果。

预期字段：

- 类型或格式枚举
- 原始字节数据
- 可选调试文本
- 可选元数据

例如：

- ZPL 文本字节
- TSPL 文本字节
- 含位图段的 TSPL 二进制负载

### 需要实现的后端

#### ZplBackend

职责：

- 将文本、条码、图形渲染为 ZPL
- 适合 Zebra 及兼容 ZPL 的打印机

#### TsplBackend

职责：

- 将 ASCII 文本、条码、图形渲染为 TSPL
- 适合不涉及中文位图的简单 TSPL 标签

#### TsplBitmapBackend

职责：

- 条码和 ASCII 走原生 TSPL
- 中文文本转成位图块
- 在需要时使用 Windows 字体栅格化策略

这个后端直接对应当前已经验证通过的 XP-360B 路径。

#### PreviewBackend

职责：

- 把标签渲染成预览图或中间模型
- 为后续视觉回归测试提供基础

## 中文文本策略

中文打印必须被视为一级设计问题。

### 为什么需要特殊处理

不同打印机在以下方面差异很大：

- 原生字体支持
- 编码支持
- Unicode 支持
- 位图黑白极性规则

### 策略模型

后端应在以下两种方式间做选择：

- 原生打印机文本
- 位图栅格化

选择依据应来自打印机档案能力和调用方请求的渲染模式。

### XP-360B 当前已验证规则

对当前接入的 `Xprinter XP-360B`，生产可用路径为：

- 使用 TSPL
- 中文文本转成位图
- 位图字节极性采用调试后确认可用的规则

这条规则应保存在基于档案的后端逻辑中，而不是散落在临时脚本里。

## 传输层设计

渲染与发送应保持解耦。

### 建议接口

```cpp
class IPrintTransport {
public:
    virtual ~IPrintTransport() = default;
    virtual void send(const PrintJob& job, const PrinterConnection& connection) = 0;
};
```

### 建议支持的传输方式

#### WindowsRawTransport

通过 WinSpool 进行 RAW 打印。

这层最终应吸收当前 `RawTSPL.ps1` 和 `RawZPL.ps1` 中的逻辑。

#### Tcp9100Transport

用于支持监听 9100 端口的网络打印机。

#### FileTransport

将输出写入文件，方便调试、预览和集成。

## 模板系统

模板系统应让业务项目更容易接入。

### 模板职责

- 定义标签结构
- 定义字段位置
- 定义版式与字体默认值
- 接受类型化输入数据

### 示例：医疗标签模板

```cpp
struct MedicalLabelData {
    std::string sampleNo;
    std::string barcodeValue;
    std::string patientId;
    std::string timestamp;
    std::u8string testItem;
    std::u8string patientName;
    std::u8string specimenType;
    std::u8string department;
};
```

```cpp
LabelDocument buildMedicalLabel(const MedicalLabelData& data);
```

这样业务代码就只需要关注数据，而不是打印机语法。

## 建议的代码组织

```text
include/
  labelprint/
    document.h
    elements.h
    layout.h
    template.h
    printer_profile.h
    backend.h
    transport.h
    preview.h

src/
  core/
  layout/
  backends/
    zpl/
    tspl/
    preview/
  transports/
    windows_raw/
    tcp9100/
    file/
  profiles/
  templates/

examples/
  medical_label/
  xp360b_print/
  zebra_zd888/

tests/
  core/
  layout/
  backends/
```

## 对外 API 方向

库应同时提供低层和高层两种使用方式。

### 低层 API

适合需要精细控制的调用方：

```cpp
LabelDocument doc({400, 240});
doc.addText(...);
doc.addBarcode(...);
```

### 高层模板 API

适合常规业务场景：

```cpp
MedicalLabelData data = ...;
LabelDocument doc = buildMedicalLabel(data);
```

### 渲染与发送

```cpp
auto profile = PrinterProfiles::xprinter_xp_360b();
TsplBitmapBackend backend;
PrintJob job = backend.render(doc, profile);

WindowsRawTransport transport;
transport.send(job, PrinterConnection{"Xprinter XP-360B #2"});
```

## 测试策略

测试应分层进行。

### 第 1 层：模型测试

验证：

- 文档能否正确保存元素
- 布局辅助是否正确放置元素

### 第 2 层：后端测试

验证：

- ZPL 输出是否包含预期命令
- TSPL 输出是否包含预期命令
- 位图后端是否生成稳定的结构

### 第 3 层：快照测试

验证：

- 渲染文本输出是否与预期快照一致
- 预览图是否保持视觉稳定

### 第 4 层：人工打印机认证

为每种支持的打印机维护一份小型认证清单。

## 实施路线图

实现应分阶段推进。

### 阶段 1：抽离核心模型 ✅ 已完成 (2026-05-17)

交付物：

- [x] `LabelDocument` → `include/labelprint/document.h`
- [x] 元素类型 → `include/labelprint/elements.h` (`TextElement`, `BarcodeElement`, `LineElement`, `BoxElement`)
- [x] 通用设置结构 → `labelprint::LabelSettings`
- [x] 与原始命令生成的明确分界 → `src/backends/detail.h` + backend classes

目标：

把当前零散的生成器提升成可复用文档模型。

### 阶段 2：引入打印机档案 ✅ 已完成 (2026-05-17)

交付物：

- [x] `PrinterProfile` → `include/labelprint/printer_profile.h`
- [x] 内置档案定义 → `PrinterProfiles::xprinter_xp360b()`, `PrinterProfiles::zebra_zd888()`, `PrinterProfiles::godex_g500u()`
- [x] 能力标记 → `nativeChinese`, `bitmapWhiteIsOne`, `textStrategy` 等字段
- [x] DPI 参数化 → `dotsToMm(dots, dpi)`, `dotsToTsplScale(dots, dpi)`

目标：

停止把打印机行为硬编码到脚本和各处逻辑里。

### 阶段 3：建立后端接口 ✅ 已完成 (2026-05-17)

交付物：

- [x] `IPrinterBackend` → `include/labelprint/backend.h`
- [x] `PrintJob` → `include/labelprint/backend.h`
- [x] `ZplBackend` → `src/backends/zpl_backend.h` + `zpl_backend.cpp`
- [x] `TsplBackend` → `src/backends/tspl_backend.h` + `tspl_backend.cpp`
- [x] `EzplGb2312Backend` → `src/backends/ezpl_gb2312_backend.cpp` (Godex G500U, Z1/Z2 + CP936)
- [x] 共享内部转换 → `src/backends/detail.h` (mapFont, convertSettings, populate)
- [x] `main.cpp` 更新为通过后端渲染
- [x] `CMakeLists.txt` 更新

目标：

把渲染职责收敛到稳定接口后面。

### 阶段 4：正式吸纳 XP-360B 已验证路径 ✅ 已完成 (2026-05-17)

交付物：

- [x] `TsplBitmapBackend` → `src/backends/tspl_bitmap_backend.h` + `.cpp`
- [x] GDI+ 字体栅格化 → `renderTextToBitmap()` (ASCII/CJK 自动分流)
- [x] 位图极性匹配已验证规则 → `whiteIsOne = true` (来自 profile)
- [x] `ZplLabel` 扩展 → `BitmapField` + `generateTsplBinary()` (支持二进制 BITMAP 数据)
- [x] `detail::populateTsplBitmap()` → 逐元素文字策略决策
- [x] `main.cpp` 中文标签测试 → u8 字面量 + `TsplBitmapBackend`
- [x] 文本策略枚举已接通 → `TextRenderMode` / `TextStrategy` / `shouldUseBitmap()`

目标：

把现在真实可用的打印链路变成正式库能力。

### 阶段 5：建立模板 API ✅ 已完成 (2026-05-17)

交付物：

- [x] `MedicalLabelData` → `include/labelprint/template.h`
- [x] `buildMedicalLabel()` → `src/templates/medical_label.cpp`
- [x] `main.cpp` 全部改用模板 API
- [x] 数据驱动标签构建 — 业务代码只需填充数据，不关心布局

目标：

给业务代码一个干净的领域接口。

### 阶段 6：增加传输实现 ✅ 已完成 (2026-05-18)

交付物：

- [x] `IPrintTransport` + `PrinterConnection` → `include/labelprint/transport.h`
- [x] `FileTransport` → `src/transports/file_transport.cpp` (写入文件，方便调试)
- [x] `WindowsRawTransport` → `src/transports/windows_raw_transport.cpp` (WinSpool RAW 打印)
- [x] `Tcp9100Transport` → `src/transports/tcp9100_transport.cpp` (TCP 9100 网络打印)
- [x] `main.cpp` 使用 `FileTransport` 替代 `saveJob()` 辅助函数

目标：

用统一发送接口支持多种目标。

### 阶段 7：增加测试基础设施 ✅ 已完成 (2026-05-18)

交付物：

- [x] `test/test_utils.h` — 共享测试宏 (`ADD_TEST`, `ASSERT`, `ASSERT_EQ`) + inline registry
- [x] `test/test_main.cpp` — 测试运行器入口，遍历 registry 执行所有测试
- [x] `test/test_document.cpp` — 11 项文档模型测试 (LabelDocument, LabelSettings, 元素, Profile)
- [x] `test/test_backends.cpp` — 24 项后端测试 (ZPL/TSPL/EZPL/TsplBitmapBackend 输出, PrintJob)
- [x] `CMakeLists.txt` — `test_runner` 构建目标，链接 gdiplus.lib
- [x] 全部 35 项测试通过

目标：

让库具备可持续演进的基础。

## 第一里程碑的非目标

第一里程碑不应试图一次解决所有问题。

当前暂不纳入范围：

- 完整的可视化 WYSIWYG 编辑器
- 复杂的自动流式布局引擎
- 完整跨平台字体栅格化一致性
- 图形化打印机管理界面

## 立即下一步

阶段 1–7 已完成。后续方向：

- `PreviewBackend` — 标签预览和快照测试
- 更多打印机档案
- 跨平台位图渲染
