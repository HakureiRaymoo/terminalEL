# terminalEL 首版交付流程（草稿）

更新于 2026-10-15。v0.1.0 作为首个发行版本，已完成本地安装包基础冒烟测试。
远端上传由维护者执行，以 GitHub Releases 实际资产为准。
terminalEL 是 Windows Terminal 的独立 fork；本 fork 的实现、文档和开发
脚本包含经维护者授权的 AI 辅助工作。上游许可证与第三方声明继续适用。

## v0.1.0 本地安装包结果（更新）

已按维护者指令完成 Release/x64 构建及 Inno Setup EXE 制作；本文下方早期
便携 ZIP 提案保留作历史参考，当前交付格式为按用户安装的 EXE。

- 编译器：官方 Inno Setup 6.7.3，安装于 `D:\Dev\sometools\InnoSetup6`；
  下载文件 Authenticode 签名有效，发布者 Pyrsys B.V.。
- 入口脚本：`tools/Package-TerminalEL.ps1`；安装定义：`tools/TerminalEL-Setup.iss`。
- 产物：`bin/terminalEL-release/0.1.0/TerminalEL-0.1.0-windows-x64-setup.exe`，
  同目录 `SHA256SUMS.txt` 记录校验值。
- 默认安装目录：`%LOCALAPPDATA%\Programs\TerminalEL`，以 `.portable` 隔离配置。
- 当前用户 App Paths 注册 `wtel.exe`，Win+R 输入 `wtel` 启动；不修改 PATH 或 `wt`。
- 本机静默安装成功；通过 ShellExecute 启动 `wtel.exe -w new` 后，进程来自
  实际安装目录、窗口响应正常并显示 PowerShell，独立 settings.json 已生成。
- 尚未在干净机器验证依赖、手动 Win+R、卸载/升级保留行为或安装后 IME 完整矩阵。
  安装包未代码签名，Windows 可能显示未知发布者。
- 正式产物须在 README/安装源码提交后重新打包，包内 build 元数据应为
  `sourceModified=false`，并记录与 v0.1.0 标签一致的源码提交。

## 当前状态与待定项

- Foundation v1 保持冻结并可恢复；Native/Tone、设置和 IME 的进展见交接记录。
- 维护者已确认候选框、输入响应、设置、鼠标选词及辉光正常；最新交互加固
  后反馈“没有问题”。没有逐项提交的 DPI/IME 矩阵结果，不将其标记为全部通过。
- panel-v1 可选网格及校准页已有实现，交接中仍缺少明确的运行验收结论。
- **待维护者确定**：首版版本号、便携 ZIP 或其他包格式、panel-v1 是否纳入首版。
- 建议首个交付目标为 **Release/x64 便携 ZIP**。该建议尚未成为发布决定。
  fork 版本标签和 EXE 内部版本号需分别记录，不假定两者已经同步。

## 1. 源码与只读检查

在仓库根目录运行现有检查：

```powershell
python tools/Test-TerminalELFoundation.py
python tools/Test-TerminalELNative.py
python tools/Test-TerminalELUI.py
python tools/Test-TerminalELPanel.py
python tools/Test-TerminalELProjectGraph.py
git diff --check
```

审阅拟交付的全部源码差异、AI 辅助披露和发布说明。工作区仍包含多轮未提交
修改；构建包对应的提交/工作区状态必须在发布记录中准确说明。
上述检查不代替 C++/WinRT、XAML、FXC 编译或实际 GPU/IME 验收。

## 2. 维护者构建 Release/x64

手动关闭该输出目录的运行实例，再从其他终端执行：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\tools\Build-TerminalEL.ps1 -Configuration Release
```

脚本依次构建 OpenConsole、WindowsTerminal，并执行
`_WTPrepareUnpackagedLayoutForRun`，包含 WinUI 资源准备和 PRI 合并。
成功输出应为 `bin\x64\Release\WindowsTerminal`；构建日志在
`obj\terminalEL-build-logs`。Debug 输出及其运行成功不能代替 Release 包验收。

## 3. 在独立暂存目录准备运行包

1. 使用新的暂存目录，保持运行目录的文件相对路径。不要直接压缩整个仓库或
   整个 bin 目录。以构建产生的完整 Release 运行布局为起点；保留所需 EXE、
   DLL、PRI、清单和资源。`_xaml` 提取缓存、stamp、PDB、LIB、EXP、OBJ 及构建
   日志不属于常规运行包；过滤后必须重新启动验证完整性。
2. 检查构建脚本所列运行必需文件，尤其 `Microsoft.UI.Xaml.dll`、
   `resources.pri`、`OpenConsole.exe`。这只是基础存在性检查，不是完整依赖扫描。
3. 如果确定采用便携包，在暂存目录的 `Microsoft.Terminal.Settings.Model.dll`
   旁创建空文件 `.portable`。非打包运行时设置将写入同级 `settings` 目录；该位置
   需要可写。此行为已由 `TerminalSettingsModel/FileUtils.cpp` 确认。
4. 首次测试使用空的 `settings` 目录。保留构建生成的默认配置资源，避免把开发机
   的 settings.json、state.json、历史记录或账号/路径数据带入分发包。
5. 放入根目录 `LICENSE`、`NOTICE.md`，以及本 fork 的发布说明和使用说明。
   配色示例可随包提供，但不要用维护者的完整配置替代用户默认配置。
6. 在复制出的暂存目录启动，确认实际进程路径来自该目录。运行时生成的
   `settings` 属于验收数据；最终 ZIP 应来自尚未运行、没有这些数据的暂存副本。

没有 `.portable` 的非打包进程仍使用 `%LOCALAPPDATA%\Microsoft\Windows Terminal`
设置目录，可能与既有非打包 Terminal 共享配置；分发说明必须说明选定模式。
便携包不自动获得 MSIX 的安装注册、更新或资源管理器集成。

## 4. 对实际包做冒烟验收

- 解压到新的可写目录；启动、关闭、再次启动，确认设置保存在预期位置。
- 新建标签、分屏、运行 PowerShell/cmd，检查字体、图标、设置页和中英文资源。
- Foundation v1 恢复、Native 色彩、Tone/Bloom、全局/Profile 继承和 Reset。
- IME 默认关闭时系统候选正常；开启后按 `terminalEL-ime-acceptance.md` 验证。
- 对首版纳入的 panel-v1 执行 `terminalEL-panel-calibration.md` 的校准对照。
- 尽可能在未安装本机开发环境的目标机器验证依赖。缺少 VC++ 运行库等依赖时，
  记录明确的安装前提；不要仅依据开发机启动成功宣称自包含。

记录 Windows 版本、GPU/驱动、缩放、构建配置、EXE 文件版本及实际结果。
当前不宣称新的最低系统版本或 ARM64 支持。

## 5. 归档与发布记录

确认版本后可使用 `terminalEL-<version>-windows-x64-portable.zip` 命名。
发布记录应含源码提交、构建工具链、包内版本、SHA-256、已知限制和测试范围。
先验证最终 ZIP 在新目录解压后的运行结果，再按维护者的明确指令执行
提交、推送、标签和 GitHub Release。包内容与对应源码版本应保持一致。

发布说明草稿见 `terminalEL-release-notes.md`。
