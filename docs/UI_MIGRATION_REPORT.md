# GnssView UI 组件迁移与兼容性交付报告 (Migration Report)

**分支**: `feature/ui-modernization`  
**初始基线**: `0a04617` (Release V0.1.5)  
**状态**: Phase 0 基线阶段完成，准备进入 Phase 1  

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
