# GnssView UI Design System 规范与设计标准

**版本**: 1.0.0  
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
| **Neutral 0** | `#ffffff` | `#06090d` | 最底层背景 / 暗室基底 |
| **Neutral 1** | `#f8fbfe` | `#080d12` | 窗口次级背景 / 面板底座 |
| **Neutral 2** | `#eef4fb` | `#0d151b` | 卡片 / 控件背景 (Surface 1) |
| **Neutral 3** | `#e2edf6` | `#111b23` | 悬停态背景 (Surface 2) |
| **Neutral 4** | `#cddfe9` | `#17252f` | 激活 / 选中态背景 (Surface 3) |
| **Accent Primary** | `#0b76d8` | `#3ba2ff` | 主交互动作、重要焦点、高亮标签 |
| **Accent Strong** | `#005fb8` | `#45a6ff` | 强强调色 (按钮按下、关键指标) |
| **Accent Soft** | `#d5ebff` | `#18354b` | 选中项半透明衬底 |
| **Danger / Bad** | `#d32f2f` | `#ff5c6a` | 错误、警报、断开、危险删除 |
| **Warning** | `#d97706` | `#f59e0b` | 警告、未定位、失步、弱信号 |
| **Success / OK** | `#16a34a` | `#22c55e` | 定位成功、已连接、就绪、校验通过 |

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
| `controlHeight` | 26px - 28px | 32px |
| `controlRadius` | 4px | 6px |
| `panelRadius` | 6px | 8px |
| `buttonPaddingHorizontal` | 10px | 14px |
| `fieldPaddingHorizontal` | 8px | 10px |
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
| **Fix: 3D Fix / DGPS** | `#16a34a` | `#22c55e` | 具备可靠解算 |
| **Fix: RTK Float** | `#0284c7` | `#38bdf8` | 浮点解状态 |
| **Fix: RTK Fixed** | `#059669` | `#34d399` | 毫米级固定解高精度 |
| **Fix: No Fix** | `#dc2626` | `#f87171` | 无定位解 |
| **Raw RX Stream** | `#168a61` | `#43eaa7` | 接收串口/网络下发报文 |
| **Raw TX Stream** | `#c45b2d` | `#ff9b70` | 发送指令报文 |

---

## 4. 统一控件库设计体系 (AppControls Architecture)

为解决 5+ 个面板重复定义组件的问题，在 `src/ui/qml/controls/` (或兼容路径 `components/`) 下提供标准控件体系：

### 4.1 核心控件矩阵与 API 标准

#### `AppButton`
- **属性**:
  - `property string variant`: `"primary"` | `"secondary"` | `"outline"` | `"ghost"` | `"danger"` | `"success"` (默认 `"secondary"`)
  - `property string size`: `"compact"` | `"normal"` (默认 `"compact"`)
  - `property color accent`: 自定义语义强调色 (向后兼容 `NeonButton.accent`)
  - `property bool loading`: 异步任务转圈状态
  - `property url iconSource`: 可选图标
- **状态行为**:
  - Hover: 轻微高亮外边框与渐变增亮
  - Pressed: 内部深度压暗，下边框高亮线反馈
  - Focused: 呈现 2px 半透明 `accentPrimary` 焦点轮廓，支持键盘 Space / Enter 激活
  - Disabled: 不透明度 0.45，禁用鼠标与键盘事件

#### `AppTextField`
- **属性**:
  - `property bool hasError`: 校验错误标识
  - `property string errorMessage`: 悬浮/辅助错误文字
  - `property string placeholderText`: 占位符
  - `property bool clearButtonEnabled`: 是否带一键清空图标
- **交互规范**:
  - 拥有焦点时边框呈现清晰的强调色 `inputBorderFocus`
  - 校验失败时边框变更为 `danger` 色并显示提示

#### `AppComboBox`
- **属性**:
  - `property var model`: 数据模型
  - `property string textRole`: 显示字段
  - `property int maxVisibleItems`: 展开下拉最大行数 (默认 8)
- **交互规范**:
  - 下拉 Popup 具备自适应滚动条，按键 Up/Down 导航，Enter 确认，Esc 收起

#### `AppCheckBox` / `AppSwitch`
- **交互规范**:
  - 勾选与滑块具备平滑动效 (60ms - 90ms)
  - 明确支持键盘 Tab 聚焦与 Space 切换

#### `AppCard` / `GlassPanel`
- **交互规范**:
  - 统一边框厚度 1px，圆角 6px - 8px，顶栏标题 14px SemiBold，支持折叠或右上角动作按钮组

---

## 5. 跨平台窗口与菜单策略决策

| 平台 | 窗口外观策略 | 菜单栏 (MenuBar) 策略 | 依据与风险控制 |
|---|---|---|---|
| **macOS** | 原生无边框带交通灯，`MacWindowStyler` 配合 | 原生系统 MenuBar (`ApplicationWindow.menuBar`) | 遵守 macOS 桌面规范；保持标准 Command 快捷键 |
| **Windows** | 优雅半定制标题栏 (保留系统 Snap/缩放) | 窗口内顶部 MenuBar 或折叠式应用菜单 | 确保 Win+Left/Right 分屏、多屏高 DPI 缩放与双击最大化绝对可靠 |
| **Linux (Wayland)** | 原生窗口边框 或 严格遵循 `startSystemMove/Resize` 的自绘 | 窗口内标准 MenuBar | Wayland 下禁止绝对坐标拖动；保证 Compositor 窗口吸附正常 |
| **Linux (X11)** | 同 Linux Wayland | 窗口内标准 MenuBar | 兼容各种轻量级 WM 与分屏平铺管理器 |

### 5.1 状态栏 (AppStatusBar) 规范
- 位于 `ApplicationWindow.footer`
- 字段编排（左至右）：
  1. 通信链路状态指示点 (UART/TCP/UDP 绿色已连 / 红色断开 / 灰色未开)
  2. 实时数据率 (RX: xx B/s, TX: xx B/s)
  3. GNSS 定位模式 (No Fix / 3D Fix / RTK Fixed)
  4. 追踪卫星数 / 参与解算数 (Sats: 24/38)
  5. PDOP / HDOP 数值指示
  6. 弹性间距 Item
  7. 软件版本与更新提示 (可点击)
- 小窗口（宽度 < 1280px）时自动折叠次要指标 (如 DOP/数据率)，优先保留链路状态与定位状态。
