# TerminalEL 开发归档与恢复指南

归档日期：2026-10-15。维护者决定停止在当前机器维护开发工作区，保留已安装程序。
本目录保存两个开发阶段的说明与工具快照，供将来重新克隆仓库后恢复开发。
快照是历史资料，不是主分支功能或执行权限的扩展；根目录 AGENTS.md 仍是开发约束入口。

## 分支与版本

| 来源 | 归档前提交 | 范围 |
| --- | --- | --- |
| main | f7e0f59dc | Foundation v1 单色显示、设置、预设及项目首页 |
| feature/terminal-ime-adaptation | d555f13c6 | Native/Tone、panel-v1、输入法适配、v0.1.0 安装器 |
| v0.1.0 标签 | d555f13c6 | 首版安装包对应源码，不移动此标签 |

main 未合并 v0.1.0 程序代码。两分支仅分别增加相同的开发归档。
`snapshots/main` 和 `snapshots/ime-adaptation` 各保留来源分支的 README、AGENTS.md、
LICENSE、NOTICE.md、terminalEL 专项文档、配色样例及开发/检查/打包脚本。
`manifest.json` 记录来源提交、文件大小与 SHA-256。原始材料不重写，旧文档中的
“待实施/待验收”应结合本文及后续交接记录判断。

## 已实现与验证范围

- Foundation v1：清晰核心、外围光晕、独立核心/辉光色、Tone/Bloom、预设与继承。
  五份冻结 shader 和认可预设必须保留，不通过更新守卫哈希掩盖算法变化。
- 适配分支另有 Native、tinted、hybrid；hybrid 按像素分类，非文字语义分类。
- TSF 负责候选、分页和提交；XAML 负责窗格内候选，Composition 字形遮罩负责辉光。
  候选默认关闭，支持设置继承、鼠标选词、空心选中框、终端字号/字重和颜色联动。
  候选辉光为视觉近似，不宣称与终端 shader 逐像素相同。
- 维护者多轮本地反馈基本可用；完整 DPI/IME 矩阵、干净机器依赖、升级/卸载仍未全部验证。
- panel-v1 默认关闭，专项验收有限。时间余辉未实施。
- Release/x64 编译和安装器生成成功；本机安装、ShellExecute wtel 启动、独立配置已验证。
  不宣称已测量 GPU 开销或全平台兼容性。

## 已安装程序与发行文件

- 程序：`%LOCALAPPDATA%\Programs\TerminalEL\WindowsTerminal.exe`。
- 配置：安装目录 `settings`，由 `.portable` 标记隔离；保留该目录和全部用户配置。
- Win+R `wtel`：HKCU App Paths 注册，不替换 wt，不修改 PATH。
- 安装工具：`D:\Dev\sometools\InnoSetup6`，官方 Inno Setup 6.7.3，保留。
- v0.1.0：`TerminalEL-0.1.0-windows-x64-setup.exe`，未代码签名。
- 正式安装器 SHA-256：`d8d7783855cdcc37a770e8d2d1e48962579431c7d8aba3b589886f5b79d5764f`。
- 包内 sourceCommit 为 d555f13c6e0c02afad07b2137f7374ffd3579cc7，sourceModified=false。
  EXE 版本字段未提取到值；发行版本以安装器/标签为准。

安装器等二进制不提交到此归档，保留在 GitHub Release 与工作区外本地归档中。
不归档个人 settings、账号数据、OpenCode 数据库、令牌或完整会话日志。

## 未来恢复开发

```powershell
git clone https://github.com/HakureiRaymoo/terminalEL.git
# 在新克隆目录执行，按所需阶段选择分支：
git switch main
# 或：git switch feature/terminal-ime-adaptation
```

先读根目录 AGENTS.md、此目录 README 和对应快照中的交接文档，检查 git status。
不得未经明确请求编译、清理已有改动或推送；保留 AI 辅助开发披露和上游许可证。
工具链：PowerShell 7、VS 2026 ≥18.6 C++/WinUI 工具、Windows SDK 26100
及对应修订版；详情见来源 README。NuGet 还原与机器环境需重新核查。

维护者明确要求构建时，从仓库根目录执行活跃的 tools 脚本，而非直接运行历史快照：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\tools\Build-TerminalEL.ps1 -Configuration Release
# 仅适配分支有打包工具：
pwsh -NoProfile -File .\tools\Package-TerminalEL.ps1 -Compiler 'D:\Dev\sometools\InnoSetup6\ISCC.exe'
```

编译前关闭对应输出目录实例，不终止所有 WindowsTerminal 进程。
保留完整运行布局：WinUI DLL、resources.pri、OpenConsole.exe。
只读检查见对应 README；通过静态检查不等同于编译/GPU/IME 验收。

## 工作区退役流程

1. 本归档分别提交到两个分支，由维护者 push；确认远端已包含两个归档提交。
2. 在项目目录外保留归档副本、清单、正式安装器和校验文件，核对散列。
3. **等待维护者明确确认 push 完成后**，检查项目无未保存改动、无程序运行于项目目录。
4. 仅删除 `D:\OpenCodestuff\terminalEL` 开发目录。保留外部归档、已安装程序、
   用户配置、开发工具以及其他项目。删除前不默认清除全局 NuGet/SDK/OpenCode 缓存。

本文件不表示删除已经发生，也不表示 GitHub Release 上传状态已经核实。
