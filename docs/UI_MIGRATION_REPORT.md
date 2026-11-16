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

- **2026-11-16 (Phase 0)**:
  - 检出 `feature/ui-modernization` 分支并完成全工程代码审计。
  - 完成 Light / Dark 双主题基线截图采集。
  - 确认构建与既有回归测试正常。
  - 架构设计方案与平台矩阵决策已就绪。
