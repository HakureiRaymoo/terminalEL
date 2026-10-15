![Windows Terminal project logos and branding image](https://github.com/microsoft/terminal/assets/91625426/333ddc76-8ab2-4eb4-a8c0-4d7b953b1179)

[![Terminal Build Status](https://dev.azure.com/shine-oss/terminal/_apis/build/status%2FTerminal%20CI?branchName=main)](https://dev.azure.com/shine-oss/terminal/_build/latest?definitionId=1&branchName=main)

# terminalEL — 终端输入法适配分支 / Terminal IME adaptation

**让终端文字具有清晰、温暖的电致发光质感。**

TerminalEL 是基于 [Windows Terminal](https://github.com/microsoft/terminal) 的实验性 fork，
受到薄膜电致发光显示屏（TFEL）视觉风格启发。在保留标签页、分屏和命令行使用方式的
基础上，加入集成式文字辉光、多种色彩模式，以及与终端协调的中文输入法候选界面。
目标是明亮清晰的文字核心、较暗且连续的外围光晕，同时兼顾代码辨色与日常阅读。

### 相比 Windows Terminal 新增的功能

- **EL 显示管线**：原生 Atlas / Direct3D 11 后处理中进行色调处理、发光提取、
  模糊和核心保护合成；亮度、对比度、黑位、Gamma 与 Bloom 均可调。
- **独立核心与辉光色**：支持核心 RGB、色相和饱和度，以及辉光色相、饱和度、混合比例。
- **四种色彩模式**：Monochrome 统一着色；Native 保留源色；Native-tinted 调整辉光色；
  Native-hybrid 将中性色映射为单色核心/独立光晕，同时保留明显彩色内容。
- **外观管理**：全局/Profile 继承、逐项 Reset、精确输入、同进程实时预览及自定义预设
  新建/更新/重命名/删除/导入/导出。保留 Foundation v1 冻结算法与内置预设。
- **终端风格输入法候选框**：字体、字重、字号与终端协调，支持单色核心与字形光晕、
  空心选中框、键盘/鼠标选词、窗格内定位与系统 UI 回退。
- **配色与校准工具**：提供 EL Amber Code 配色示例、ANSI/True Color/代码测试图、
  黑白阶与细字校准样本，以及默认关闭的 panel-v1 静态面板网格。

Windows Terminal 已有配色、透明度及自定义着色器能力；本项目新增的是这套具体的
EL 算法、设置、预设和输入法视觉适配。Native-hybrid 根据像素颜色分类，并不识别
文字语义；灰色背景与低饱和度内容也可能被映射。面板网格仍需专项运行验收，
图表是目视辅助，不是 ICC/HDR 或仪器级校准。时间余辉不在当前实现范围内。

本分支 `feature/terminal-ime-adaptation` 保存已由维护者确认基本可用的输入法适配版本。
基于 Windows Terminal，为 TSF 候选列表提供 terminalEL 风格界面，支持键盘/鼠标选词、
系统 UI 回退、Profile 设置继承，以及与终端协调的字号、字重、单色核心色和字形辉光。
选中候选使用空心细框。候选浮层及柔光默认关闭，可在全局或 Profile“外观”中开启：

```json
{
    "terminalEL.imeCandidateOverlay": true,
    "terminalEL.imeCandidateGlow": true
}
```

This branch is a terminal IME adaptation snapshot, locally exercised by the
maintainer, not a packaged release or a claim of compatibility with every IME.
It also includes the accompanying Native color, optional panel and settings work
since Foundation v1. The frozen Foundation v1 renderer remains available.
Candidate glyph glow uses Composition and is not pixel-identical to terminal shaders.

本 fork 的实现、文档和开发脚本包含经维护者授权的 AI 辅助工作。
AI-assisted development applies to this fork; upstream licenses and notices remain.

### 当前状态

这是 **基本可用、持续调整中的分支版本**，首个发行版本为 **v0.1.0 x64 EXE 安装包**。
发行文件以本仓库 Releases 中实际提供的资产为准。已完成本机基础安装与启动验证。
维护者已在本机使用微软拼音，对候选显示、输入响应、设置继承、鼠标选词、
字号/字重、颜色及辉光进行了迭代验证。其他输入法、跨显示器 DPI 和特殊布局
的兼容范围以实际测试为准，详细验收表保留尚未逐项记录的状态。

| 模块 | 当前方案 |
| --- | --- |
| Foundation v1 | 保留冻结的单色渲染算法和预设，可恢复原有效果 |
| Native 色彩 | 包含 native、native-tinted、native-hybrid 路径及 Tone/Bloom 设置 |
| 输入法候选 | 默认关闭的窗格内浮层，使用 TSF 候选数据与提交接口 |
| 外观设置 | 全局默认、Profile 覆盖、继承 Reset；候选柔光独立开关 |
| 面板与校准 | 包含默认关闭的 panel-v1 网格和校准图表；仍需专项运行验收 |

### 构造方案：TSF 数据 + XAML 候选界面 + Composition 辉光

```text
微软拼音 / TSF 候选生命周期
    └─ src/tsf：文档归属、候选分页、选中项和有效性校验
         └─ TermControl：窗格内 XAML 候选浮层
              ├─ TextBlock：清晰文字、序号与页码
              ├─ Border：空心选中框与框体柔光
              └─ Composition：字形 AlphaMask + 近层/宽层阴影

终端正文 ── Atlas / D3D 着色器 ── 核心颜色、Tone 与 Bloom
候选界面 ── 读取有效 Profile 参数 ── 对齐字体、核心色及辉光色
```

- **输入行为**：候选仍由输入法生成；键盘分页、数字/空格确认等保留原有输入法
  路径。鼠标选词调用 TSF `SetSelection` / `Finalize`，不把候选字符串直接写入 PTY。
- **生命周期与性能**：校验候选所属文档和窗格，失焦/关闭时清理；16 ms 定时器
  合并候选更新，复用行控件，空闲时不重复读取候选字符串。布局约每 100 ms 检查。
- **字体与颜色**：读取终端实际字体名称、字重和包含缩放的字号；核心色沿用单色版
  RGB、色相、饱和度和 Tone，辉光沿用独立辉光色与混合比例。中文回退字体和抗锯齿
  路径仍可能与正文略有差异。
- **辉光实现**：保留清晰文字，在后方使用字形遮罩生成近层及宽层光晕；读取 Bloom
  参数并做 Composition 强度映射。它是视觉适配，不是终端着色器的逐像素复制。
- **回退**：接口不支持、数据无效或窗格空间不足时请求显示系统候选 UI；
  高对比度模式下关闭候选装饰辉光。

### 本地构建与运行

构建环境要求见下方上游 Prerequisites；本 fork 的便捷脚本要求 **PowerShell 7、
VS 2026 ≥ 18.6 C++ 工具链**及对应 Windows SDK。以下命令由使用者按需执行：

```powershell
git switch feature/terminal-ime-adaptation

# 在仓库根目录构建 Debug/x64，准备完整运行布局并启动
pwsh -NoProfile -ExecutionPolicy Bypass -File .\tools\Build-TerminalEL.ps1 -Run

# Release/x64（不自动启动）
pwsh -NoProfile -ExecutionPolicy Bypass -File .\tools\Build-TerminalEL.ps1 -Configuration Release
```

请先关闭对应输出目录的本地实例，再从另一个终端构建。脚本不会终止进程。
运行目录为 `bin\x64\<Debug或Release>\WindowsTerminal`，构建日志位于
`obj\terminalEL-build-logs`。脚本会准备 WinUI DLL、合并 PRI 并放置 OpenConsole；
仅完成普通项目 Build 不一定具备完整运行布局。

构建后在“外观”中启用上方两个开关，保存后开始新的候选会话。
`TERMINALEL_IME_OVERLAY=1/0` 是覆盖保存设置的诊断选项；正常验收设置时应移除。
`TERMINALEL_IME_PROBE=1` 可输出 DebugView 元数据，测试输入流畅度时应关闭。

非打包运行默认使用 `%LOCALAPPDATA%\Microsoft\Windows Terminal` 配置。
若需独立配置，可在输出目录的 `Microsoft.Terminal.Settings.Model.dll` 旁创建空文件
`.portable`；新进程会使用同级 `settings` 目录。该目录需要可写，切换前请保留配置备份。

### v0.1.0 安装包与 `wtel` 入口

首版安装器按当前用户安装到 `%LOCALAPPDATA%\Programs\TerminalEL`，提供开始菜单
快捷方式和卸载入口。安装后按 **Win+R**，输入 **`wtel`** 即可打开 TerminalEL。
该入口通过当前用户的 App Paths 注册，不替换 Windows Terminal 的 `wt`，也不修改 PATH；
它用于 Windows 运行对话框/ShellExecute，不承诺在 cmd/PowerShell 内作为命令使用。

安装目录包含 `.portable` 标记，配置保存在其 `settings` 子目录。升级保留配置，卸载也
保留用户配置。安装器版本为 0.1.0，程序内部仍可能显示上游构建版本，包内
`terminalEL-build.json` 记录实际版本与源码状态。首版未使用代码签名证书，Windows
可能显示“未知发布者”。

完成 Release 构建后，使用官方 Inno Setup 编译器打包：

```powershell
pwsh -NoProfile -File .\tools\Package-TerminalEL.ps1 -Compiler 'D:\Dev\sometools\InnoSetup6\ISCC.exe'
```

安装器与 SHA-256 校验文件输出到 `bin\terminalEL-release\0.1.0`。
该脚本不编译主程序、不安装产物，也不复制开发机用户配置。

### 非编译检查

```powershell
python tools/Test-TerminalELFoundation.py
python tools/Test-TerminalELNative.py
python tools/Test-TerminalELUI.py
python tools/Test-TerminalELPanel.py
python tools/Test-TerminalELProjectGraph.py
git diff --check
```

这些脚本验证冻结基线、设置/资源/项目引用和部分算法模型，不执行 C++/WinRT、
XAML 或 HLSL 编译，也不代替实际 GPU、字体及输入法测试。

### 详细文档

- [候选框使用与实现说明](doc/terminalEL-ime-overlay.md)
- [边界验收记录](doc/terminalEL-ime-acceptance.md)
- [构建与交付流程](doc/terminalEL-release.md)

## Upstream Windows Terminal documentation

以下保留上游 Windows Terminal 文档。Microsoft Store、winget 和上游 Releases
安装的是上游产品，**不包含本分支的 terminalEL 输入法适配**；本分支目前按上文从源码构建。
The upstream build badge above describes the upstream pipeline, not validation of this fork.

# Welcome to the Windows Terminal, Console and Command-Line repo

<details>
  <summary><strong>Table of Contents</strong></summary>

- [Installing and running Windows Terminal](#installing-and-running-windows-terminal)
  - [Microsoft Store \[Recommended\]](#microsoft-store-recommended)
  - [Other install methods](#other-install-methods)
    - [Via GitHub](#via-github)
    - [Via Windows Package Manager CLI (aka winget)](#via-windows-package-manager-cli-aka-winget)
    - [Via Chocolatey (unofficial)](#via-chocolatey-unofficial)
    - [Via Scoop (unofficial)](#via-scoop-unofficial)
- [Installing Windows Terminal Canary](#installing-windows-terminal-canary)
- [Terminal \& Console Overview](#terminal--console-overview)
  - [Windows Terminal](#windows-terminal)
  - [The Windows Console Host](#the-windows-console-host)
  - [Shared Components](#shared-components)
  - [Creating the new Windows Terminal](#creating-the-new-windows-terminal)
- [Resources](#resources)
- [FAQ](#faq)
  - [I built and ran the new Terminal, but it looks just like the old console](#i-built-and-ran-the-new-terminal-but-it-looks-just-like-the-old-console)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Communicating with the Team](#communicating-with-the-team)
- [Developer Guidance](#developer-guidance)
- [Prerequisites](#prerequisites)
- [Building the Code](#building-the-code)
  - [Building in PowerShell](#building-in-powershell)
  - [Building in Cmd](#building-in-cmd)
- [Running \& Debugging](#running--debugging)
  - [Coding Guidance](#coding-guidance)
- [Code of Conduct](#code-of-conduct)

</details>

<br />

This repository contains the source code for:

* [Windows Terminal](https://aka.ms/terminal)
* [Windows Terminal Preview](https://aka.ms/terminal-preview)
* The Windows console host (`conhost.exe`)
* Components shared between the two projects
* [ColorTool](./src/tools/ColorTool)
* [Sample projects](./samples)
  that show how to consume the Windows Console APIs

Related repositories include:

* [Windows Terminal Documentation](https://learn.microsoft.com/windows/terminal)
  ([Repo: Contribute to the docs](https://github.com/MicrosoftDocs/terminal))
* [Console API Documentation](https://github.com/MicrosoftDocs/Console-Docs)
* [Cascadia Code Font](https://github.com/Microsoft/Cascadia-Code)

## Installing and running Windows Terminal

> [!NOTE]
> Windows Terminal requires Windows 10 2004 (build 19041) or later

### Microsoft Store [Recommended]

Install the [Windows Terminal from the Microsoft Store][store-install-link].
This allows you to always be on the latest version when we release new builds
with automatic upgrades.

This is our preferred method.

### Other install methods

#### Via GitHub

For users who are unable to install Windows Terminal from the Microsoft Store,
released builds can be manually downloaded from this repository's [Releases
page](https://github.com/microsoft/terminal/releases).

Download the `Microsoft.WindowsTerminal_<versionNumber>.msixbundle` file from
the **Assets** section. To install the app, you can simply double-click on the
`.msixbundle` file, and the app installer should automatically run. If that
fails for any reason, you can try the following command at a PowerShell prompt:

```powershell
# NOTE: If you are using PowerShell 7+, please run
# Import-Module Appx -UseWindowsPowerShell
# before using Add-AppxPackage.

Add-AppxPackage Microsoft.WindowsTerminal_<versionNumber>.msixbundle
```

> [!NOTE]
> If you install Terminal manually:
>
> * You may need to install the [VC++ v14 Desktop Framework Package](https://learn.microsoft.com/troubleshoot/cpp/c-runtime-packages-desktop-bridge#how-to-install-and-update-desktop-framework-packages).
>   This should only be necessary on older builds of Windows 10 and only if you get an error about missing framework packages.
> * Terminal will not auto-update when new builds are released so you will need
>   to regularly install the latest Terminal release to receive all the latest
>   fixes and improvements!

#### Via Windows Package Manager CLI (aka winget)

[winget](https://github.com/microsoft/winget-cli) users can download and install
the latest Terminal release by installing the `Microsoft.WindowsTerminal`
package:

```powershell
winget install --id Microsoft.WindowsTerminal -e
```

> [!NOTE]
> Dependency support is available in WinGet version [1.6.2631 or later](https://github.com/microsoft/winget-cli/releases). To install the Terminal stable release 1.18 or later, please make sure you have the updated version of the WinGet client.

#### Via Chocolatey (unofficial)

[Chocolatey](https://chocolatey.org) users can download and install the latest
Terminal release by installing the `microsoft-windows-terminal` package:

```powershell
choco install microsoft-windows-terminal
```

To upgrade Windows Terminal using Chocolatey, run the following:

```powershell
choco upgrade microsoft-windows-terminal
```

If you have any issues when installing/upgrading the package please go to the
[Windows Terminal package
page](https://chocolatey.org/packages/microsoft-windows-terminal) and follow the
[Chocolatey triage process](https://chocolatey.org/docs/package-triage-process)

#### Via Scoop (unofficial)

[Scoop](https://scoop.sh) users can download and install the latest Terminal
release by installing the `windows-terminal` package:

```powershell
scoop bucket add extras
scoop install windows-terminal
```

To update Windows Terminal using Scoop, run the following:

```powershell
scoop update windows-terminal
```

If you have any issues when installing/updating the package, please search for
or report the same on the [issues
page](https://github.com/lukesampson/scoop-extras/issues) of Scoop Extras bucket
repository.

---

## Installing Windows Terminal Canary
Windows Terminal Canary is a nightly build of Windows Terminal. This build has the latest code from our `main` branch, giving you an opportunity to try features before they make it to Windows Terminal Preview.

Windows Terminal Canary is our least stable offering, so you may discover bugs before we have had a chance to find them.

Windows Terminal Canary is available as an App Installer distribution and a Portable ZIP distribution.

The App Installer distribution supports automatic updates. Due to platform limitations, this installer only works on Windows 11.

The Portable ZIP distribution is a portable application. It will not automatically update and will not automatically check for updates. This portable ZIP distribution works on Windows 10 (19041+) and Windows 11.

| Distribution  | Architecture    | Link                                                 |
|---------------|:---------------:|------------------------------------------------------|
| App Installer | x64, arm64, x86 | [Download](https://aka.ms/terminal-canary-installer) |
| Portable ZIP  | x64             | [Download](https://aka.ms/terminal-canary-zip-x64)   |
| Portable ZIP  | ARM64           | [Download](https://aka.ms/terminal-canary-zip-arm64) |
| Portable ZIP  | x86             | [Download](https://aka.ms/terminal-canary-zip-x86)   |

_Learn more about the [types of Windows Terminal distributions](https://learn.microsoft.com/windows/terminal/distributions)._

---

## Terminal & Console Overview

Please take a few minutes to review the overview below before diving into the
code:

### Windows Terminal

Windows Terminal is a new, modern, feature-rich, productive terminal application
for command-line users. It includes many of the features most frequently
requested by the Windows command-line community including support for tabs, rich
text, globalization, configurability, theming & styling, and more.

The Terminal will also need to meet our goals and measures to ensure it remains
fast and efficient, and doesn't consume vast amounts of memory or power.

### The Windows Console Host

The Windows Console host, `conhost.exe`, is Windows' original command-line user
experience. It also hosts Windows' command-line infrastructure and the Windows
Console API server, input engine, rendering engine, user preferences, etc. The
console host code in this repository is the actual source from which the
`conhost.exe` in Windows itself is built.

Since taking ownership of the Windows command-line in 2014, the team added
several new features to the Console, including background transparency,
line-based selection, support for [ANSI / Virtual Terminal
sequences](https://en.wikipedia.org/wiki/ANSI_escape_code), [24-bit
color](https://devblogs.microsoft.com/commandline/24-bit-color-in-the-windows-console/),
a [Pseudoconsole
("ConPTY")](https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/),
and more.

However, because Windows Console's primary goal is to maintain backward
compatibility, we have been unable to add many of the features the community
(and the team) have been wanting for the last several years including tabs,
unicode text, and emoji.

These limitations led us to create the new Windows Terminal.

> You can read more about the evolution of the command-line in general, and the
> Windows command-line specifically in [this accompanying series of blog
> posts](https://devblogs.microsoft.com/commandline/windows-command-line-backgrounder/)
> on the Command-Line team's blog.

### Shared Components

While overhauling Windows Console, we modernized its codebase considerably,
cleanly separating logical entities into modules and classes, introduced some
key extensibility points, replaced several old, home-grown collections and
containers with safer, more efficient [STL
containers](https://docs.microsoft.com/en-us/cpp/standard-library/stl-containers?view=vs-2022),
and made the code simpler and safer by using Microsoft's [Windows Implementation
Libraries - WIL](https://github.com/Microsoft/wil).

This overhaul resulted in several of Console's key components being available
for re-use in any terminal implementation on Windows. These components include a
new DirectWrite-based text layout and rendering engine, a text buffer capable of
storing both UTF-16 and UTF-8, a VT parser/emitter, and more.

### Creating the new Windows Terminal

When we started planning the new Windows Terminal application, we explored and
evaluated several approaches and technology stacks. We ultimately decided that
our goals would be best met by continuing our investment in our C++ codebase,
which would allow us to reuse several of the aforementioned modernized
components in both the existing Console and the new Terminal. Further, we
realized that this would allow us to build much of the Terminal's core itself as
a reusable UI control that others can incorporate into their own applications.

The result of this work is contained within this repo and delivered as the
Windows Terminal application you can download from the Microsoft Store, or
[directly from this repo's
releases](https://github.com/microsoft/terminal/releases).

---

## Resources

For more information about Windows Terminal, you may find some of these
resources useful and interesting:

* [Command-Line Blog](https://devblogs.microsoft.com/commandline)
* [Command-Line Backgrounder Blog
  Series](https://devblogs.microsoft.com/commandline/windows-command-line-backgrounder/)
* Windows Terminal Launch: [Terminal "Sizzle
  Video"](https://www.youtube.com/watch?v=8gw0rXPMMPE&list=PLEHMQNlPj-Jzh9DkNpqipDGCZZuOwrQwR&index=2&t=0s)
* Windows Terminal Launch: [Build 2019
  Session](https://www.youtube.com/watch?v=KMudkRcwjCw)
* Run As Radio: [Show 645 - Windows Terminal with Richard
  Turner](https://www.runasradio.com/Shows/Show/645)
* Azure DevOps Podcast: [Episode 54 - Kayla Cinnamon and Rich Turner on DevOps
  on the Windows
  Terminal](http://azuredevopspodcast.clear-measure.com/kayla-cinnamon-and-rich-turner-on-devops-on-the-windows-terminal-team-episode-54)
* Microsoft Ignite 2019 Session: [The Modern Windows Command Line: Windows
  Terminal -
  BRK3321](https://myignite.techcommunity.microsoft.com/sessions/81329?source=sessions)

---

## FAQ

### I built and ran the new Terminal, but it looks just like the old console

Cause: You're launching the incorrect solution in Visual Studio.

Solution: Make sure you're building & deploying the `CascadiaPackage` project in
Visual Studio.

> [!NOTE]
> `OpenConsole.exe` is just a locally-built `conhost.exe`, the classic
> Windows Console that hosts Windows' command-line infrastructure. OpenConsole
> is used by Windows Terminal to connect to and communicate with command-line
> applications (via
> [ConPty](https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/)).

---

## Documentation

All project documentation is located at [aka.ms/terminal-docs](https://aka.ms/terminal-docs). If you would like
to contribute to the documentation, please submit a pull request on the [Windows
Terminal Documentation repo](https://github.com/MicrosoftDocs/terminal).

---

## Contributing

We are excited to work alongside you, our amazing community, to build and
enhance Windows Terminal\!

***BEFORE you start work on a feature/fix***, please read & follow our
[Contributor's
Guide](./CONTRIBUTING.md) to
help avoid any wasted or duplicate effort.

## Communicating with the Team

The easiest way to communicate with the team is via GitHub issues.

Please file new issues, feature requests and suggestions, but **DO search for
similar open/closed preexisting issues before creating a new issue.**

If you would like to ask a question that you feel doesn't warrant an issue
(yet), please reach out to us via Twitter:

* Christopher Nguyen, Product Manager:
  [@nguyen_dows](https://twitter.com/nguyen_dows)
* Dustin Howett, Engineering Lead: [@dhowett](https://twitter.com/DHowett)
* Mike Griese, Senior Developer: [@zadjii@mastodon.social](https://mastodon.social/@zadjii)
* Carlos Zamora, Developer: [@cazamor_msft](https://twitter.com/cazamor_msft)
* Pankaj Bhojwani, Developer
* Leonard Hecker, Developer: [@LeonardHecker](https://twitter.com/LeonardHecker)

## Developer Guidance

## Prerequisites

You can configure your environment to build Terminal in one of two ways:

### Using WinGet configuration file

After cloning the repository, you can use a [WinGet configuration file](https://learn.microsoft.com/en-us/windows/package-manager/configuration/#use-a-winget-configuration-file-to-configure-your-machine)
to set up your environment. The [default configuration file](.config/configuration.winget) installs Visual Studio 2026 Community & rest of the required tools. There are two other variants of the configuration file available in the [.config](.config) directory for Enterprise & Professional editions of Visual Studio 2026. To run the default configuration file, you can either double-click the file from explorer or run the following command:

```powershell
winget configure .config\configuration.winget
```

### Manual configuration

* You must be running Windows 10 2004 (build >= 10.0.19041.0) or later to run
  Windows Terminal
* You must [enable Developer Mode in the Windows Settings
  app](https://learn.microsoft.com/windows/uwp/get-started/enable-your-device-for-development)
  to locally install and run Windows Terminal
* You must have [PowerShell 7 or later](https://github.com/PowerShell/PowerShell/releases/latest) installed
* You must have the [Windows 11 (10.0.26100) SDK](https://developer.microsoft.com/windows/downloads/windows-sdk/) installed at version 10.0.26100.8249 or greater.
* You must have at least [VS 2026](https://visualstudio.microsoft.com/downloads/) version 18.6 installed
* You must install the following Workloads via the VS Installer. Note: Opening
  the solution will [prompt you to install missing components automatically](https://devblogs.microsoft.com/setup/configure-visual-studio-across-your-organization-with-vsconfig/):
  * Desktop Development with C++
  * WinUI application development
* You must install the [.NET Framework 4.7.2 Targeting Pack](https://learn.microsoft.com/dotnet/framework/install/guide-for-developers#to-install-the-net-framework-developer-pack-or-targeting-pack) to build test projects

## Building the Code

OpenConsole.slnx may be built from within Visual Studio or from the command-line
using a set of convenience scripts & tools in the **/tools** directory:

### Building in PowerShell

```powershell
Import-Module .\tools\OpenConsole.psm1
Set-MsBuildDevEnvironment
Invoke-OpenConsoleBuild
```

### Building in Cmd

```shell
.\tools\razzle.cmd
bcz
```

## Running & Debugging

To debug the Windows Terminal in VS, right click on `CascadiaPackage` (in the
Solution Explorer) and go to properties. In the Debug menu, change "Application
process" and "Background task process" to "Native Only".

You should then be able to build & debug the Terminal project by hitting
<kbd>F5</kbd>. Make sure to select either the "x64" or the "x86" platform - the
Terminal doesn't build for "Any Cpu" (because the Terminal is a C++ application,
not a C# one).

> 👉 You will _not_ be able to launch the Terminal directly by running the
> WindowsTerminal.exe. For more details on why, see
> [#926](https://github.com/microsoft/terminal/issues/926),
> [#4043](https://github.com/microsoft/terminal/issues/4043)

### Coding Guidance

Please review these brief docs below about our coding practices.

> 👉 If you find something missing from these docs, feel free to contribute to
> any of our documentation files anywhere in the repository (or write some new
> ones!)

This is a work in progress as we learn what we'll need to provide people in
order to be effective contributors to our project.

* [Coding Style](./doc/STYLE.md)
* [Code Organization](./doc/ORGANIZATION.md)
* [Exceptions in our legacy codebase](./doc/EXCEPTIONS.md)
* [Helpful smart pointers and macros for interfacing with Windows in WIL](./doc/WIL.md)

---

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of
Conduct][conduct-code]. For more information see the [Code of Conduct
FAQ][conduct-FAQ] or contact [opencode@microsoft.com][conduct-email] with any
additional questions or comments.

[conduct-code]: https://opensource.microsoft.com/codeofconduct/
[conduct-FAQ]: https://opensource.microsoft.com/codeofconduct/faq/
[conduct-email]: mailto:opencode@microsoft.com
[store-install-link]: https://aka.ms/terminal
