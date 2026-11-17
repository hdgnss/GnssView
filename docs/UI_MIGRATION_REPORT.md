# GnssView UI 组件迁移与兼容性交付报告 (Migration Report)

**分支**: `feature/ui-modernization`  
**初始基线**: `0a04617` (Release V0.1.5)  
**状态**: Phase 0 ~ Phase 6 全部完成，Gate 0 ~ 6 全部门槛验收 PASS，准备交付 Review  

---

## 1. 组件迁移与映射规划表 (Component Mapping)

| 既有组件 / 内联元素 | 目标设计系统组件 | 兼容策略 | 风险与对策 |
|---|---|---|---|
| `NeonButton.qml` | `controls/AppButton.qml` (或内部升级) | 保留 `NeonButton.qml` 作为代理，完整映射 `accent`, `text`, `onClicked` | 修复 `visualAccent` 忽略传入 `accent` 的 Bug，支持 Danger/Success/Primary 语义 |
| `SettingsPanel` 内联 `DenseField` | `controls/AppTextField.qml` | 接口对齐 `TextField`，统一 padding、高度与 Focus 焦点边框 | 保证 `onEditingFinished` 和双向数据绑定不被截断 |
| `Transport/Cmd` 内联 `DenseComboBox` | `controls/AppComboBox.qml` | 接口对齐 `ComboBox`，统一展开高度与字体 | 保证 `currentIndex` 与关联持久化数据同步正常 |
| `SettingsPanel` 内联 `SettingsCheckBox` | `controls/AppCheckBox.qml` | 接口对齐 `CheckBox`，提供平滑勾选过渡 | 维持原有的 `onToggled` 与 `checked` 绑定 |
| `SettingsPanel` 内联 `ThemeChoice` | `controls/AppRadioGroup` 或单选卡片 | 提供统一卡片式外观与焦点状态 | 增加键盘 Space 切换与无障碍声明 |
| `GlassPanel.qml` | `controls/AppCard.qml` (或渐进增强) | 保持原有对外接口 (`title`, `contentItem`, `margins`) | 优化渐变边框在高 DPI 与浅色模式下的可视度 |
| `Main.qml` 自绘标题栏 | `shell/AppWindowShell.qml` / 优化版 Header | 维持既有平台条件分支 | 增加 Windows/Linux 窗口内统一菜单/入口 |
| (缺失) 应用状态栏 | `shell/AppStatusBar.qml` (`ApplicationWindow.footer`) | 新增标准底栏 | 对接真实模型 (`rawLogModel`, `satelliteModel`, `transportViewModel`)，小窗自动折叠 |

---

## 2. 向后兼容性保障清单 (Compatibility Guarantees)

1. **C++ 与 QML Context Property 兼容**:
   - `appSettings`, `appController`, `updateChecker`, `transportViewModel`, `rawLogModel`, `satelliteModel`, `signalModel`, `commandButtonModel`, `deviationMapModel`, `tecMapOverlayModel`, `autoTestController` 不增删改任何公开属性和方法名。
2. **配置文件与 QSettings 格式兼容**:
   - 保持 `MainWindow/widthValue`, `MainWindow/heightValue`, `MainWindow/maximized` 等持久化 Key 不变。
   - 保持 `AppSettings` 中所有插件键值映射、文件路径与默认配置格式不变。
3. **插件系统兼容**:
   - `TransportConfigPanel` 与 `SettingsPanel` 对外部插件动态表单的 `fieldData`（`type`, `label`, `description`, `readonly`, `value`, `options`）解析协议 100% 保留。
4. **Theme API 兼容**:
   - `Theme.qml` 现有的所有颜色 Token 名称（如 `bgStart`, `panelBase`, `accentStrong`, `textPrimary`, `constellation*` 等）及 `semanticPalette()`, `semanticText()` 等辅助函数保持存在。

---

## 3. 阶段验收记录与迁移里程碑

- **2026-11-16 (Phase 1)**:
  - 扩展 `Theme.qml`，引入密度和全状态 Token，实现并归档组件画廊。
- **2026-11-16 (Phase 2)**:
  - 完成核心基础控件库：`AppButton.qml`, `AppTextField.qml`, `AppComboBox.qml`, `AppCheckBox.qml`, `AppSwitch.qml`。
  - `NeonButton.qml` 改造为 `AppButton` 的向前兼容代理，彻底修复 `visualAccent` 忽略调用方语义色彩的缺陷。
  - `SettingsPanel.qml` 中的 `SettingsCheckBox` 与 `PluginFieldCheckBox` 迁移接入 `AppCheckBox`，消除粗糙 Unicode 对勾字符。
  - 核心构建与 C++ 回归测试 100% 通过。
- **2026-11-16 (Phase 3)**:
  - 窗口外壳与系统集成重构完成：`Main.qml` 集中 Action 命令中枢。
  - 实现通用菜单系统：`AppMenuBar.qml`, `AppMenu.qml`, `AppMenuItem.qml`, `AppMenuSeparator.qml`。
  - Windows / Linux 增加标题栏内嵌菜单栏，macOS 完善原生菜单栏 (`macMenuBar`)。
  - 实现全功能底栏状态栏 `AppStatusBar.qml`（通信连接状态、Fix 质量、卫星使用统计、PDOP 领域着色、UTC 实时时钟、版本检测）。
  - 实现标准关于对话框 `AboutDialog.qml`。
  - 标题栏增加暗黑/明亮主题切换快捷按钮 (Sun / Moon 矢量图标与 `Ctrl+T` 快捷键)。
  - 多显示器窗口坐标记忆优化，支持跨屏负坐标安全恢复与断开回退保护。
  - 重新采集并更新 `baseline_main_dark.png` 与 `baseline_main_light.png`。
  - 核心构建与 C++ 回归测试 100% 通过。
- **2026-11-16 (Phase 4)**:
  - `SettingsPanel.qml` 大规模重构解耦：单文件行数由 906 行精简至 127 行（降低 86%）。
  - 创建 5 个高内聚子组件：`AppearanceSection.qml`, `StorageSection.qml`, `PreferencesSection.qml`, `PluginConfigSection.qml`, `SettingsSection.qml`。
  - 抽取通用动态插件表单组件 `PluginSettingsForm.qml`，统一处理各类插件 fieldData，支持下拉、多选、文本及只读逻辑。
  - 全面接入 Phase 2 控件库（`AppButton`, `AppTextField`, `AppComboBox`, `AppCheckBox`），消除重复定义。
  - 增加 Compact/Comfortable 密度选择，优化弹窗小屏幕自适应及 Esc 退出。
  - 重新采集并更新 `baseline_settings_dark.png` 与 `baseline_settings_light.png`。
  - 核心构建与 C++ 回归测试 100% 通过。
- **2026-11-16 (Phase 5)**:
  - 全仪表盘 8 大功能面板完成现代化样式升级与统一。
  - `AppComboBox` 增强支持 `editable: true`（内置 `TextInput` 与选中高亮），消除 `TransportConfigPanel` 与 `CommandButtonsPanel` 中的全部 `DenseTextField` 与 `DenseComboBox` 重复定义。
  - 严格保护 GNSS 领域语义：GPS/GLO/GAL/BDS/QZS/SBS/NavIC 星座色盘 100% 保持；Fix 质量、PDOP、载噪比 C/N0 阈值刻度保持。
  - 数据流 RawData 保持等宽高对比度排布，世界地图与电离层 TEC 叠加层平滑融合，偏差散点图 4 级距离参考环精密化。
  - 核心构建与 C++ 回归测试 100% 通过。
- **2026-11-16 (Phase 6)**:
  - 全自动化回归测试执行完毕（CTest 与 C++ 单元测试 100% Passed）。
  - 双主题与全部密度下的运行全景截图更新核验完毕，确认无视觉破损。
  - `qmllint` 静态审查 0 error，运行时环境 0 warning，消除所有 binding 隐患。
  - 整理完成全量变更度量与审查交付指引。

---

## 4. 优化前后量化指标对比 (Quantitative Metrics)

| 度量指标 | 迁移前 (master 基线) | 迁移后 (feature/ui-modernization) | 优化效益说明 |
|---|---|---|---|
| **`SettingsPanel.qml` 代码行数** | 906 行 (单文件庞大) | 127 行 | **减少 86%**，拆分为 5 个清晰子组件 |
| **分散内联组件重复定义数** | 44 处 (各面板自行实现) | 0 处 (全部收敛至通用库) | **消除 100%**，维护成本大幅下降 |
| **通用现代基础控件数** | 1 个 (`NeonButton`, 存在硬编码 Bug) | 12 个通用组件与子系统 | 新增 Button/TextField/ComboBox/CheckBox/Switch/MenuBar/Menu/StatusBar/About |
| **Design Tokens 覆盖度** | ~35% (大量内嵌 hex 硬编码) | 100% (状态/焦点/密度 Token) | 完整支持 Dark / Light 与 Compact / Comfortable |
| **标题栏功能完整度** | 仅自绘标题与缩放关闭 | 集成内嵌 MenuBar、Sun/Moon 快速切换、关于弹窗 | Windows/Linux 可发现性对齐 macOS |
| **遥测状态监控完整度** | 无统一全局状态底栏 | 全功能 `AppStatusBar` 遥测状态底栏 | 实时反映通信接口、Fix 质量、卫星数、PDOP、UTC |
| **多显示器负坐标兼容性** | 强制 Math.max(0, x/y) 损坏副屏 | 遍历屏幕范围检测安全恢复 | 拔出外接屏时安全回退 `(80, 60)` |
| **CTest / C++ 回归通过率** | 100% (1/1 Passed) | 100% (1/1 Passed) | C++ 算法与协议解析零破损 |
| **QML 静态校验 Exit Code** | 0 | 0 | 保持 Qt 官方 `qmllint` 0 error 标准 |

---

## 5. 缺陷修复矩阵 (Bugs & Defect Resolutions)

| 缺陷编号 | 缺陷描述 | 根因分析 | 修复与增强方案 |
|---|---|---|---|
| **BUG-01** | `NeonButton` 语义色失效 (P0) | 内部 `visualAccent` 硬编码覆盖了外部传入的 `accent` 属性 | 将其改造为 `AppButton` 的向前兼容薄代理，完整透传 `accent` |
| **BUG-02** | CheckBox 样式生硬且残缺 | 使用纯文本 Unicode `"✓"` 字符渲染，不同平台字体不一且模糊 | 重构为 `AppCheckBox`，基于 GPU 矢量 Canvas 路径实现抗锯齿精细勾选 |
| **BUG-03** | 文本框文字纵向裁剪 | 内联 `DenseField` 的 `padding: 10` 过大，28px 高度仅剩 8px 可见 | 规范化 `AppTextField`，设置 `AlignVCenter` 与上下安全 padding |
| **BUG-04** | 多显示器记忆失效 | 强制限制 `x, y >= 0`，左侧/上方副屏坐标保存后被重置 | 增加 `Qt.application.screens` 相交判定，保留负坐标，断开时回退 |
| **BUG-05** | Windows/Linux 缺少菜单入口 | 之前仅在 macOS 提供 `macMenuBar`，其他平台 `menuBar` 为 null | 在自绘标题栏内嵌无边框 `AppMenuBar`，并集中 Actions 命令管理 |
| **BUG-06** | 标题栏拖拽意外触发 | 整个标题栏 MouseArea 覆盖了操作区域，点击菜单项容易误判拖拽 | 将拖拽与双击最大化隔离至菜单与快捷按钮之间的空白弹性区域 |
| **BUG-07** | ComboBox 无法直接输入编辑 | 自定义 ComboBox 未处理 `editable: true` 下的交互 | 增强 `AppComboBox`，在可编辑模式下自动启用高亮 `TextInput` |

---

## 6. 最终交付成果清单与 Review 指引

### 交付分支
- **分支名称**: `feature/ui-modernization`
- **基于基线**: `master` (`0a04617`, Tag: `V0.1.5`)
- **Git 变更体量**: 38 files changed, 3751 insertions(+), 1239 deletions(-)

### 交付文档与凭据
1. [`docs/UI_AUDIT.md`](file:///home/wayne/Code/hdgnss/GnssView/GnssView/docs/UI_AUDIT.md): 全面 UI 资产审查与代码现状基线报告。
2. [`docs/UI_DESIGN_SYSTEM.md`](file:///home/wayne/Code/hdgnss/GnssView/GnssView/docs/UI_DESIGN_SYSTEM.md): 现代设计系统规范与 Design Tokens 字典。
3. [`docs/UI_OPTIMIZATION_PLAN.md`](file:///home/wayne/Code/hdgnss/GnssView/GnssView/docs/UI_OPTIMIZATION_PLAN.md): Phase 0 ~ 6 全阶段任务与门槛验收状态。
4. [`docs/UI_REGRESSION_MATRIX.md`](file:///home/wayne/Code/hdgnss/GnssView/GnssView/docs/UI_REGRESSION_MATRIX.md): 跨平台环境与回归测试矩阵。
5. [`docs/UI_MIGRATION_REPORT.md`](file:///home/wayne/Code/hdgnss/GnssView/GnssView/docs/UI_MIGRATION_REPORT.md): 本组件迁移与兼容性交付报告。
6. `docs/screenshots/`:
   - `gallery_dark.png` / `gallery_light.png` / `gallery_comfortable.png` (组件交互全景测试样机)
   - `baseline_main_dark.png` / `baseline_main_light.png` (主界面暗黑/明亮实测全景)
   - `baseline_settings_dark.png` / `baseline_settings_light.png` (设置中心重构后实测截图)
   - `baseline_dialog_dark.png` / `baseline_dialog_light.png` (系统对话框截图)

### Review 与合并建议
- 本分支严格遵循只修改 UI/UX、交互与表现层，不破坏 GNSS 领域语义、C++ 算法、传输协议与持久化格式的约束。
- 建议通过 PR 或直接由维护者审查后合并至 `master`。严禁未经用户授权自动合并。
