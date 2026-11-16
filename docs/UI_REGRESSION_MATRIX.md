# GnssView UI 回归验证与实测矩阵 (UI Regression Matrix)

**最后更新**: 2026-11-16  
**当前状态**: Phase 1 完成 (Gate 1 通过)  

---

## 1. 自动化回归测试基线 (Automated Test Baseline)

| 测试套件 / 可执行文件 | 命令 | 结果 | 耗时 | 备注 |
|---|---|---|---|---|
| `GnssViewRegression` | `./build/GnssViewRegression` | **PASS** | 0.8s | 包含 AppController, SatelliteStore, NMEA 解算等核心逻辑测试 |
| `GnssViewStreamChunkerRegression` | `./build/GnssViewStreamChunkerRegression` | **PASS** | 0.2s | 流数据分片与校验回归测试 |
| `transport_discovery_regression` | `ctest -R transport_discovery_regression` | **PASS** | 2.46s | CTest 注册的传输层插件探测测试 |
| `qmltestrunner (GenericTransportUi)` | `qmltestrunner -input tests/qml/tst_GenericTransport.qml` | **BASELINE FAIL** | 0.15s | 上游 commit `56b5a49` 在 line 60 断言失败，确认为原有基线问题，已记录追踪 |

---

## 2. 界面视觉基线截图库 (Visual Screenshot Baseline)

所有基线截图均已在真实 Linux 运行时下完成采集并归档至 `docs/screenshots/`：

| 截图文件路径 | 视图 | 主题 | 尺寸 | 验证内容说明 |
|---|---|---|---|---|
| `docs/screenshots/baseline_main_dark.png` | 主窗口全景 | Dark | 3840x2160 (HiDPI @2x) | 4x3 面板布局、网格背景、自绘顶栏标题与窗口控制按钮 |
| `docs/screenshots/baseline_main_light.png` | 主窗口全景 | Light | 3840x2160 (HiDPI @2x) | 亮色背景、面板边框、文字可读性与仪表盘色彩适配 |
| `docs/screenshots/baseline_settings_dark.png` | 设置中心弹窗 | Dark | 3840x2160 (HiDPI @2x) | 外观单选、偏好复选框、插件列表与字段表单 |
| `docs/screenshots/baseline_settings_light.png` | 设置中心弹窗 | Light | 3840x2160 (HiDPI @2x) | 亮色模式下设置中心各表单控件对比度与间距 |
| `docs/screenshots/baseline_dialog_dark.png` | 更新通知弹窗 | Dark | 3840x2160 (HiDPI @2x) | 弹窗遮罩、发布日志滚动区、Later/Download 按钮表现 |
| `docs/screenshots/baseline_dialog_light.png` | 更新通知弹窗 | Light | 3840x2160 (HiDPI @2x) | 亮色遮罩与对话框边框层次 |
| `docs/screenshots/gallery_dark.png` | 组件画廊测试样机 | Dark | 1920x1290 | 暗色模式下控件矩阵、GNSS 领域色及状态联动验证 |
| `docs/screenshots/gallery_light.png` | 组件画廊测试样机 | Light | 1920x1290 | 亮色模式下对比度、边框清晰度与焦点环验证 |
| `docs/screenshots/gallery_comfortable.png` | 组件画廊测试样机 | Light (Comfortable) | 1920x1290 | 舒适密度排版、控件高 34px 与间距自适应验证 |

---

## 3. 平台实测与验证覆盖矩阵 (Platform Matrix)

| 平台环境 | 测试状态 | 验证细节 | 遗留风险与指引 |
|---|---|---|---|
| **Linux (Wayland, GNOME)** | **TESTED** | Qt 6.11.2, gcc 16, 双主题切换即时响应, HiDPI 缩放 | 自绘标题栏通过 `startSystemMove/Resize` 响应，双击最大化正常 |
| **Linux (X11)** | **NOT TESTED** | 本机当前处于 Wayland 会话，未直接切换 X11 会话实测 | 后续可通过 `QT_QPA_PLATFORM=xcb` 模拟验证 |
| **macOS (Apple Silicon / Intel)** | **NOT TESTED** | 本地为 Linux 环境，无法直接执行 macOS 编译与运行 | 代码层面需严格保护 `menuBar: window.isMac ? macMenuBar : null` 与 `MacWindowStyler` |
| **Windows 10/11** | **NOT TESTED** | 本地无 Windows 运行环境 | 代码层面需保持现有 `useThemedChrome` 条件分支与 Windows 编译宏 |

---

## 4. 验收矩阵标准追踪 (Acceptance Criteria Tracking)

- [x] **Design Tokens 基线**: 确认当前散落硬编码颜色与内嵌控件现状
- [x] **双主题切换**: 确认现有 `uiTheme` 通过 `AppSettings` 驱动并能在暗黑/明亮间切换
- [x] **Theme.qml 增强与向后兼容**: 保持 100% 现有对外 API，扩充焦点/状态/密度 Token
- [x] **Control Gallery 画廊验证**: 集中展示双主题、各交互状态与领域色彩
- [x] **通用控件库 (Phase 2)**: 完成 AppButton, AppTextField, AppComboBox, AppCheckBox, AppSwitch，修复 NeonButton 语义色缺陷
- [x] **窗口外壳与系统集成 (Phase 3)**:
  - [x] 统一命令 Action 中枢 (`openSettingsAction`, `checkUpdatesAction`, `toggleThemeAction`, `toggleDensityAction`, `toggleFullscreenAction`, `aboutAction`, `quitAction`)
  - [x] Windows / Linux 内嵌菜单栏 (`AppMenuBar`, `AppMenu`, `AppMenuItem`, `AppMenuSeparator`)
  - [x] macOS 原生菜单栏 (`macMenuBar`) 结构补全并保持向前兼容
  - [x] 真实遥测底栏 (`AppStatusBar`): 通信连接、Fix 模式、卫星追踪数、PDOP (领域着色)、UTC 时钟、版本与更新检测
  - [x] 标题栏交互: 增加 Dark/Light 快速切换按钮 (带快捷键 `Ctrl+T` 提示)，多屏负坐标安全恢复
  - [x] 关于对话框 (`AboutDialog`): 版本、Qt 运行时、平台信息展示
- [x] **核心回归测试**: 100% 运行通过，无破坏既有模型与算法
