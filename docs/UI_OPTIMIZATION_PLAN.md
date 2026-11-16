# GnssView UI 优化分阶段执行计划 (Optimization Plan)

**分支**: `feature/ui-modernization` (基于 `origin/master` @ `0a04617`)  
**状态**: Phase 1 完成 (Gate 1 通过)，准备进入 Phase 2  
**上次更新**: 2026-11-16  

---

## 阶段进展概览

| 阶段 | 描述 | 状态 | 目标交付物 |
|---|---|---|---|
| **Phase 0** | 全量审计与基线建立 (只读审查与测试基线) | **DONE (Gate 0 通过)** | `UI_AUDIT.md`, 基线截图, 测试日志, 风险表 |
| **Phase 1** | Design System 规范落地与 Theme 演进 | **DONE (Gate 1 通过)** | `UI_DESIGN_SYSTEM.md`, `Theme.qml` 增强, `ControlGallery.qml` 及全景截图 |
| **Phase 2** | 通用控件库实现与 NeonButton 收敛 | **DONE (Gate 2 通过)** | `AppButton`, `NeonButton` 代理, `AppTextField`, `AppComboBox`, `AppCheckBox`, `AppSwitch` |
| **Phase 3** | 跨平台主窗口 Shell、菜单栏与统一状态栏 | **TODO (下一阶段)** | `Main.qml` Shell 重构, Linux/Win MenuBar, Footer 状态栏 |
| **Phase 4** | SettingsPanel 模块化拆分与重构 | **TODO** | 设置区块组件化, 动态插件表单独立, 键盘无障碍 |
| **Phase 5** | 全 Dashboard 面板现代样式迁移 | **TODO** | 8 大面板样式迁移, 领域语义色保护, 高 DPI 布局 |
| **Phase 6** | 跨平台回归验证、交付报告与文档归档 | **TODO** | `UI_REGRESSION_MATRIX.md`, `UI_MIGRATION_REPORT.md` |

---

## 详细任务 Checklist

### Phase 0 — 全量审计与基线 (只读，禁止改动源码)
- [x] 确认 Git 分支：从 `origin/master` 新建 `feature/ui-modernization` 并检出
- [x] 检查构建系统、Qt 版本 (Qt 6.11.2, GCC 16.2.1, CMake 3.24+, Ninja)
- [x] 扫描所有 QML 文件并梳理依赖图谱 (`docs/UI_AUDIT.md`)
- [x] 审计 `Main.qml`, `Theme.qml`, `NeonButton.qml`, `SettingsPanel.qml` 及各面板组件重复代码
- [x] 运行全量构建与现有自动化回归测试 (`GnssViewRegression`, `GnssViewStreamChunkerRegression`, `transport_discovery_regression`)
- [x] 记录现有测试基线状况 (标记 `tst_GenericTransport.qml` 历史基线失败点)
- [x] 捕获当前版本 Light/Dark 界面基线截图 (主窗口、Settings、更新弹窗)
- [x] 产出风险清单与平台策略决策依据

**Gate 0 验收结果**: **PASS** (环境具备、代码可编译可运行、测试跑通、基线截图完整留存)

---

### Phase 1 — Design System 规范与设计样机
- [x] 完善 `docs/UI_DESIGN_SYSTEM.md`，确立 Token、尺寸、状态矩阵
- [x] 验证跨平台 QQuickStyle (`Basic` 样式兼容性确定，记录决策依据)
- [x] 重构/扩展 `src/ui/qml/components/Theme.qml`，保留所有向后兼容属性与函数，增补语义 Token 与密度支持
- [x] 建立轻量级组件对照画廊 (`src/ui/qml/components/ControlGallery.qml`)
- [x] 采集画廊截图：暗黑全景 (`gallery_dark.png`)、明亮全景 (`gallery_light.png`)、舒适密度 (`gallery_comfortable.png`)
- [x] 验证 Light/Dark 切换时所有 Token 实时双向同步，既有核心回归测试 100% 通过

**Gate 1 门槛验收**: **PASS** (旧 Theme 对外 API 100% 兼容，控件测试画廊交互完备，无任何回归问题)

---

### Phase 2 — 通用控件库
- [x] 实现 `AppButton.qml`，支持 `variant` (primary/secondary/outline/ghost/danger/success)，支持自定义 `accent`
- [x] 将 `NeonButton.qml` 封装为对 `AppButton` 的向前兼容薄代理，修复 `visualAccent` 忽略传入 `accent` 的 Bug
- [x] 实现 `AppTextField.qml`，集成 Focus 状态、错误校验提示 (`hasError`) 与 Monospace 等宽支持
- [x] 实现 `AppComboBox.qml`，提供统一的下拉弹出、Chevron 旋转动效与悬停反馈
- [x] 实现 `AppCheckBox.qml` 与 `AppSwitch.qml`，废除 Unicode 伪字符，采用矢量抗锯齿路径
- [x] 在 `ControlGallery.qml` 中建立控件全状态与双主题交互测试矩阵 (Hover / Focus / Disabled / Pressed)

**Gate 2 门槛验收**: **PASS** (所有基础控件在双主题下状态一致，修复了 NeonButton 语义色缺陷，旧组件使用方 100% 兼容)

---

### Phase 3 — 跨平台主窗口、菜单和状态栏
- [ ] 重构 `Main.qml` 窗口架构：解耦 Actions 为单一命令中枢
- [ ] 补齐 Windows / Linux 窗口内菜单栏或汉堡菜单入口，确保 Settings 与更新功能可发现
- [ ] 确保 macOS 原生 MenuBar 与快捷键不受破坏
- [ ] 实现统一应用底栏状态栏 (`ApplicationWindow.footer`)，对接真实通讯与解算状态
- [ ] 优化小窗口 (1280x720) 及高 DPI 缩放下的边缘自适应

**Gate 3 门槛**: 三平台窗口行为与菜单操作无死角，底栏状态数据真实可靠。

---

### Phase 4 — SettingsPanel 重构
- [ ] 将 1017 行的 `SettingsPanel.qml` 拆分为清晰的子区块文件
- [ ] 抽取通用控件，消除 `DenseField/SettingsCheckBox/PluginComboBox` 等重复实现
- [ ] 封装独立动态插件表单组件 (`PluginSettingsForm.qml`)，保障原有字段读写与只读逻辑不变
- [ ] 改造 `ThemeChoice` 为无障碍单选控件，支持键盘 Space 选择
- [ ] 约束弹窗尺寸以适应小屏幕/低分辨率，确保 Esc 关闭正常

**Gate 4 门槛**: 所有已有配置项与插件动态设置保存/读取 100% 兼容。

---

### Phase 5 — 全 Dashboard / 面板样式迁移
- [ ] 迁移 `GlassPanel.qml` 卡片样式与统一 Header
- [ ] 迁移 `TransportConfigPanel.qml` (替换重复控件，保留通信配置与插件加载)
- [ ] 迁移 `DataPanel.qml` (统一指标卡片与数据流高亮)
- [ ] 迁移 `SkyViewPanel.qml` (保留星空极坐标色彩与图例，优化暗深色层级)
- [ ] 迁移 `CommandButtonsPanel.qml` (优化指令按钮排布与响应)
- [ ] 迁移 `SignalBarsPanel.qml` (优化载噪比柱状图与频段切换体验)
- [ ] 迁移 `WorldLocationPanel.qml` (优化世界地图与 TEC 叠加层呈现)
- [ ] 迁移 `DeviationMapPanel.qml` (散点图精密化显示)
- [ ] 迁移 `AutoTestPanel.qml`

**Gate 5 门槛**: 8 大面板无功能退化，实时刷新高帧率保持，无 QML binding 断链。

---

### Phase 6 — 真实运行回归、文档与交付
- [ ] 运行全套 CTest 与 C++ / QML 回归测试
- [ ] 捕获迁移后 Light/Dark 最终界面截图与基线对比
- [ ] 检查并消除新增 QML 运行时 warnings
- [ ] 产出 `UI_REGRESSION_MATRIX.md` 与 `UI_MIGRATION_REPORT.md`
- [ ] 提交代码评审与变更归档
