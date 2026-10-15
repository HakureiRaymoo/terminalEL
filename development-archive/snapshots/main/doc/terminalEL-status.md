# terminalEL 状态与后续开发

最新源码更新：2026-10-12。本轮 UI 未运行编译或 GUI。下文工具链与输出布局记录来自 2026-10-11 的历史核查，不代表当前输出目录状态。最新预设行为与人工验收项见 `terminalEL-foundation-v1.md`。

## 目标与已实现范围

目标是基于 Windows Terminal 的 TFEL 显示系统。已确认的产品方向：全局默认 + Profile 覆盖；Tone/Bloom 调整即时预览，Save 写入 JSON，未保存关闭设置页撤销；技术选择兼顾性能和显示效果。

| 模块 | 当前实现 |
| --- | --- |
| D3D 显示链路 | 原始帧 → 单通道 emission → 半分辨率提取 → 横/纵模糊 → 磷光色合成 |
| 参数模型 | 16 项数值含核心／辉光色彩；Native Color 尚未实现 |
| 继承 | Profile 本层/父层（含 profiles.defaults）优先，最终回退到全局 |
| 设置界面 | 全局 13 项数值输入；Profile 八项 Tone/Bloom 滑块及五项色彩滑块／输入框、独立辉光开关、逐项继承 Reset；简体中文资源 |
| 预设 | 四个只读内置预设、自定义新建／更新／重命名／删除／导入／导出、全局参数匹配状态 |
| 实时预览 | 33 ms 合并更新，同一进程内跨窗口；不涵盖独立进程 |
| 已有测试 | `TerminalSettingsTests::TFELInheritance`；历史会话记录测试通过，本次未执行 |
| 尚待实现 | Native Color、时间余辉、面板特征、显示校准 |

主要入口：

- `src/cascadia/TerminalSettingsModel/MTSMSettings.h`
- `src/cascadia/TerminalSettingsAppAdapterLib/TerminalSettings.cpp`
- `src/cascadia/TerminalApp/TFELPreview.h`
- `src/cascadia/TerminalControl/ControlCore.cpp`
- `src/renderer/atlas/BackendD3D.cpp` 及 `tfel_*.hlsl`

## 早期渲染修正（Foundation v1 冻结前）

1. Bloom shader 的半径上限由 8 px 对齐到 UI 的 16 px。原来 8–16 的滑块区间没有增加模糊半径；默认 8 px 的核不变。
2. Bloom intensity <= 0 时跳过提取及两个模糊 pass，只执行 emission 与合成，TFEL 后处理由 5 次 draw 降到 2 次。复用现有纹理，避免拖动强度时反复分配；没有据此宣称帧率提升或显存降低。
3. 添加只读预检脚本 `tools/Test-TerminalELPrerequisites.ps1`。

## 构建和运行布局核查

### 当前环境

- 检测到 `D:\Dev\VisualStudio\2026\BuildTools`，VS 18.10.2。
- SDK 实际位于 `C:\Program Files (x86)\Windows Kits\10`，不是 VS 安装目录。
- 26100 SDK headers/libs、fxc、makepri、XAML targets、.NET Framework 4.7.2 引用程序集及 pwsh 命令可找到。
- `dotnet --list-sdks` 当前列出 10.0.401。README 另要求 SDK servicing revision >= 10.0.26100.8249，不能只凭目录 `10.0.26100.0` 判断修订号。
- 历史报错的 WpfTerminalControl、TerminalStress、WpfTerminalTestNetCore 三个项目当前均已有 `obj/project.assets.json`，不应再当作仍缺失来处理。

只读预检（不调用 MSBuild）：

```powershell
& 'D:\OpenCodestuff\terminalEL\tools\Test-TerminalELPrerequisites.ps1' |
    Format-Table -Wrap -AutoSize
```

脚本检查文件存在性，不证明工具可用、还原资产有效或所有源文件可编译。全解决方案附带工具/测试项目的缺失资产报告，不等于主程序的必然阻塞。

### 已知构建风险

1. **还原源**：`NuGet.Config` 清空其他源，只使用 TerminalDependencies。历史全解决方案构建中，WpfTerminalControl、TerminalStress、WpfTerminalTestNetCore 缺少 assets，随后遇到 .NET 引用包版本无法从该源取得。历史处理是单次指定 nuget.org 还原；本次没有改源或还原。
2. **WinRT/XAML 生成依赖**：改过 IDL 后，单独构建静态库可能留下旧的 DLL WinMD/projection。应保留项目引用构建，必要时定位特定生成项目，不默认删除整个 bin/obj。中断构建留下的 `.g.h` 缺失需结合具体日志判断。
3. **运行布局**：`WindowsTerminal.vcxproj` 只在 `BuildingInsideVisualStudio=true` 时自动把 `_WTPrepareUnpackagedLayoutForRun` 加入运行准备链。命令行普通 Build 不保证合并 PRI 或提取完整 WinUI DLL。
4. **当前输出确实不完整**：核查时 `bin/x64/Debug/WindowsTerminal` 有主程序及相关 DLL，但缺少 `Microsoft.UI.Xaml.dll`、`resources.pri`、`OpenConsole.exe`。这是直接启动布局问题，不是已证实的 C++ 编译错误。
5. **输出被占用**：运行中的本地实例可能阻止 DLL 更新。手动关闭对应实例；不要运行会杀死所有 WindowsTerminal 的命令，尤其在当前终端内。
6. **时间戳**：修改发生在 DLL 时，EXE 无需重链接。检查实际 DLL、资源和构建退出码，不能由 EXE 时间戳不变推断失败。

### 用户手动构建参考

日常统一入口（默认 Debug/x64，4 个并行构建节点）：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File "D:\OpenCodestuff\terminalEL\tools\Build-TerminalEL.ps1" -Run
```

脚本会设置 VS 开发环境、按项目引用还原、增量构建 OpenConsole 与 WindowsTerminal、准备非打包资源布局并检查关键文件；任何阶段失败即停止，日志保存在 `obj/terminalEL-build-logs/<时间戳>`。`-Run` 表示成功后打开新窗口，省略则只构建。`-Plan` 只显示计划，不执行还原或构建。并行度可用 `-Jobs 2` 降低；默认不执行 Clean/Rebuild，也不结束运行中的进程。

脚本已做语法和计划模式检查，尚未实际运行编译；不能保证消除源码错误、网络还原失败或运行时故障。以下保留底层分步命令，供定位阶段性问题使用。

以下命令**本次没有执行**。在需要验收时，由用户执行；使用增量 Build 而非默认全量 Rebuild。

```powershell
$root = 'D:\OpenCodestuff\terminalEL'
$msbuild = 'D:\Dev\VisualStudio\2026\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe'
$common = @('/p:Configuration=Debug', '/p:Platform=x64', "/p:SolutionDir=$root\", '/m', '/nr:false', '/v:minimal', '/nologo')

# 独立构建的 WindowsTerminal 项目不会自动生成布局所需的 OpenConsole.exe。
& $msbuild "$root\src\host\exe\Host.EXE.vcxproj" @common /t:Build
if ($LASTEXITCODE -ne 0) { throw 'OpenConsole build failed.' }

& $msbuild "$root\src\cascadia\WindowsTerminal\WindowsTerminal.vcxproj" @common /t:Build
if ($LASTEXITCODE -ne 0) { throw 'WindowsTerminal build failed.' }

# 工程已有目标：提取 WinUI DLL，合并各依赖的 PRI，准备直接运行布局。
& $msbuild "$root\src\cascadia\WindowsTerminal\WindowsTerminal.vcxproj" @common /t:_WTPrepareUnpackagedLayoutForRun
if ($LASTEXITCODE -ne 0) { throw 'Unpackaged layout preparation failed.' }
```

上述内部目标来自当前工程源码；尚未在本轮实际验证。如果报 NuGet/WinMD 错误，先处理该具体依赖，勿直接重复全量清理重建。

### 本轮非编译验证

- `git diff --check` 通过。
- 只读预检在 Windows PowerShell 5.1 和 PowerShell 7 中执行成功；最终检查未报告工具文件缺失，报告上述三项运行布局文件缺失。
- Bloom 核算术检查通过：权重和为 1；0–8 px 支持范围保持；12/16 px 支持范围分别增长到 12/16；超过 16 截断。
- 未运行 MSBuild、FXC、C++ 编译、NuGet restore、自动化二进制测试或 GUI。算术检查不能替代 shader 编译和显卡上的视觉测试。

## 已知实现边界及优先级

### P0：先完成当前版本人工验收

- 交接记录中用户已确认个性化页闪退及后续 C3779 修复。本轮新增资源／输入绑定仍需用户构建并验收。
- 两窗口同 Profile 的滑块同步、不同 Profile 隔离、父层继承、零值覆盖、单项 Reset。
- 未保存关闭撤销；Save 后关闭及重启保留；多个设置页同时编辑时验证最新预览优先及关闭回退。
- Bloom intensity 从正值→0→正值，无残留 halo；半径 8→12→16 应有差异；默认参数效果保持。
- 窗口缩放、最小化恢复、奇数像素尺寸、高 DPI、反色文本及光标选区。

### P1：稳定参数与渲染边界

- TFEL 目前只在 BackendD3D 中实现；Direct2D（包括部分自动软件回退）不呈现 TFEL。
- Custom pixel shader/Retro 路径优先于 TFEL，当前没有组合效果。
- 合成输出 alpha 固定为 1，透明背景/背景图的显示兼容性需要专项验收。
- Renderer API 已集中限制范围并为非有限值回退默认值；数值输入和外部配置仍需检查端到端表现。
- 单色模式即使 intensity 为零仍需要全分辨率后处理，并将 dirty rect 扩到全画面。后续先测量 GPU 时间，不直接增加持续重绘。
- 预览总线仍需专门验证 Dispatcher 关闭、队列失效与 Save/reload 竞态；目前无独立预览状态机回归测试。

### P2：按已定路线继续

Profile 色彩与继承 UI 验收 → 设置页性能测量 → 稳定基线发布确认 → Native Color + TFEL Glow → Temporal Persistence → Panel Characteristics → Calibration。

建议先让 Native Color 成为明确模式而非默默改变单色默认值；彩色 emission/halo 会改变纹理格式及带宽预算。余辉必须同时设计 idle redraw、衰减截止和设备丢失重置，避免为了效果长期占用 GPU。涉及可感知的默认行为变化时先确认产品取舍。
