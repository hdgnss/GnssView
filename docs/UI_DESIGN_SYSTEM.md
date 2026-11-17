# GnssView UI Design System 规范与设计标准

**版本**: 1.1.0  
**适用范围**: Qt Quick / QML 桌面端 (macOS, Windows, Linux)  
**设计定位**: 专业级 GNSS 科学与工程数据工作台 — 现代、精密、克制、信息高密度  

---

## 1. 核心设计原则

1. **信息与语义第一 (Domain-Driven)**: GNSS 频段、星座、定位状态、DOP、电离层扰动等具备国际标准的色彩与视觉语义，任何 UI 统一不得以稀释或破坏专业语义为代价。
2. **克制且精密的视觉基调 (Precision Engineering)**: 拒绝大面积高饱和度霓虹、无意义呼吸灯及模糊光晕；采用中性冷灰背景层级配合低饱和度青蓝强调色（禁止 `#0000FF` 纯蓝）。
3. **100% 完整双主题 (Dark / Light Synchronized)**: 暗黑主题确保实验室/夜间/野外低眩光与对比度；明亮主题提供干净清晰的纸质/日照高可视度；切换时所有组件、图表、Canvas 零残留即时同步。
4. **全交互状态清晰 (Predictable States)**: Default、Hover、Pressed、Focused (键盘焦点圈)、Disabled、Loading、Error、Warning 均具备可辨识视觉差。
5. **双密度自适应 (Compact vs Comfortable)**: 默认兼容现有紧凑型高密度数据流（Compact），同时支持高分辨率/舒适模式（Comfortable）。

---

## 2. 基础设计 Token (Design Tokens)

### 2.1 基础色板 (Primitive Palette)
| Token 类别 | 浅色 (Light) | 深色 (Dark) | 描述 / 用途 |
|---|---|---|---|
| **Neutral 0 (`surface0`)** | `#f1f7fc` | `#080d12` | 最底层背景 / 暗室基底 |
| **Neutral 1 (`surface1`)** | `#ffffff` | `#0d151b` | 窗口次级背景 / 面板底座 |
| **Neutral 2 (`surface2`)** | `#eaf3fa` | `#111b23` | 卡片 / 控件背景 (Surface 2) |
| **Neutral 3 (`surface3`)** | `#dcebf6` | `#17252f` | 悬停态背景 (Surface 3) |
| **Neutral 4 (`surface4`)** | `#c5dced` | `#213947` | 激活 / 选中态背景 (Surface 4) |
| **Accent Primary** | `#0b76d8` | `#45a6ff` | 主交互动作、重要焦点、高亮标签 |
| **Accent Strong** | `#005fb8` | `#45a6ff` | 强强调色 (按钮按下、关键指标) |
| **Accent Soft** | `#d5ebff` | `#9bd8ff` | 选中项半透明衬底 |
| **Focus Ring** | `rgba(4, 70, 216, 0.35)` | `rgba(69, 166, 255, 0.45)` | 键盘导航 2px 外层焦点环 |
| **Danger / Bad** | `#b4233d` | `#ff6578` | 错误、警报、断开、危险删除 |
| **Warning** | `#b46b00` | `#f2b35f` | 警告、未定位、失步、弱信号 |
| **Success / OK** | `#147a48` | `#42d77d` | 定位成功、已连接、就绪、校验通过 |

### 2.2 文本与层级 (Typography)
- **字体族声明**:
  - `titleFont`: `Avenir Next`, `Segoe UI`, `SF Pro Display`, `Inter`, `Noto Sans`, `Ubuntu`, `Helvetica Neue`
  - `bodyFont`: `uiBodyFontFamily` 优先，回退 `SF Pro Text`, `Segoe UI`, `Helvetica Neue`, `PingFang SC`, `Arial`, `sans-serif`
  - `monoFont`: `uiMonoFontFamily` 优先，回退 `SF Mono`, `JetBrains Mono`, `Consolas`, `Menlo`, `Monaco`, `monospace`
- **字阶层级**:
  - `titleLarge`: 16px / Bold (窗口标题、主要弹窗头)
  - `titleNormal`: 14px / SemiBold (面板 Title、分区标题)
  - `bodyNormal`: 11px / Regular (主要标签、数值读数、表单)
  - `bodySmall`: 10px / Regular (辅助说明、副标题)
  - `monoNormal`: 11px / Regular (NMEA 原始报文、HEX、坐标数值、时间戳)
  - `monoSmall`: 10px / Regular (卫星方位角/高度角、微型数值)

### 2.3 尺寸与间距 (Metrics)
| Metric Token | Compact (默认紧凑) | Comfortable (舒适) |
|---|---|---|
| `controlHeight` | 28px | 34px |
| `controlRadius` | 6px | 6px |
| `panelRadius` | 8px | 8px |
| `compactSpacing` | 6px | 8px |
| `sectionSpacing` | 8px | 12px |
| `panelContentMargin` | 8px | 12px |

---

## 3. GNSS 领域专属色彩规范 (Domain Colors)

**重要规范**：不得将以下专业色彩统一为普通 Accent 色。

| 领域对象 | 浅色模式 | 深色模式 | 行业标准/含义 |
|---|---|---|---|
| **GPS (Navstar)** | `#0089b7` | `#72f2ff` | 美国 GPS 星座追踪标识 |
| **GLONASS** | `#1f8f48` | `#7df59b` | 俄罗斯 GLONASS 星座追踪标识 |
| **Galileo** | `#b77900` | `#ffd16f` | 欧洲 Galileo 星座追踪标识 |
| **BDS (BeiDou)** | `#ba4bb1` | `#ff86e9` | 中国北斗 BDS 星座追踪标识 |
| **QZSS** | `#466fd0` | `#93b8ff` | 日本 QZSS 准天顶星座标识 |
| **SBAS** | `#2e985d` | `#83f8ad` | 广域增强系统 (WAAS/EGNOS/MSAS/GAGAN) |
| **NavIC (IRNSS)** | `#c05f31` | `#ff9f73` | 印度 NavIC 星座标识 |
| **Fix: 3D Fix / DGPS** | `#147a48` | `#42d77d` | 具备可靠解算 |
| **Fix: RTK Float** | `#0b76d8` | `#45a6ff` | 浮点解状态 |
| **Fix: RTK Fixed** | `#059669` | `#34d399` | 毫米级固定解高精度 |
| **Fix: No Fix** | `#b4233d` | `#ff6578` | 无定位解 |
| **Raw RX Stream** | `#168a61` | `#43eaa7` | 接收串口/网络下发报文 |
| **Raw TX Stream** | `#c45b2d` | `#ff9b70` | 发送指令报文 |

---

## 4. Qt Quick Controls 基础样式比对与证据 (Style Selection)

在桌面端 Qt Quick 中存在多种 Controls Style，评估如下：

1. **`Basic` Style (采用)**:
   - **特点**: 最轻量、零硬编码平台外观，所有背景与绘制委托给 QML `background` 与 `contentItem`。
   - **优势**: 跨平台绝对一致，渲染开销最小，完美契合 Design Tokens 与双主题切换，无历史遗留 C++ 样式干扰。
   - **状态**: `app/main.cpp` 已声明 `QQuickStyle::setStyle("Basic")`，无适配摩擦。
2. **`Fusion` Style (未采用)**:
   - **缺点**: 包含大量 Qt 传统桌面小部件的 3D 浮雕高光和固定渐变，在高 DPI 自绘面板中容易产生双重边框冲突。
3. **`macOS / Windows` 原生 Styles (未采用)**:
   - **缺点**: 内部硬编码平台主题色彩，Qt 官方文档明确指出其不支持深度自绘扩展。

---

## 5. 组件预览画廊 (Control Gallery)

通过 [ControlGallery.qml](file:///home/wayne/Code/hdgnss/GnssView/GnssView/src/ui/qml/components/ControlGallery.qml) 提供集中式、交互式控件测试样机，包含：
- **实时模式切换**: 暗黑 (Dark) 与 明亮 (Light) 双向切换即时同步。
- **密度切换**: 紧凑 (Compact) 与 舒适 (Comfortable) 尺寸联动。
- **全状态展示**: 按钮、输入框、下拉框、复选框在 Default / Hover / Pressed / Focused / Disabled 状态的真实渲染。
- **GNSS 语义卡片**: 卫星星座 Used vs Tracked 色块对比、Fix 状态指示卡、DOP 指示卡与数据流徽章。
