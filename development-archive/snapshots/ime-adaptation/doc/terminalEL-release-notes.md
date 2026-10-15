# TerminalEL v0.1.0 — EL 显示与终端输入法适配

首个发行版本，提供 Windows x64 EXE 安装器与 SHA-256 校验文件。
安装后按 Win+R 输入 `wtel` 打开程序。安装器按当前用户安装，提供开始菜单
与卸载入口；不修改原版 `wt`。配置位于安装目录的 `settings`，升级/卸载保留。
安装器尚未代码签名，Windows 可能提示未知发布者。

terminalEL 是基于 Windows Terminal 的终端显示实验 fork，提供可调的 EL 风格
文字核心和辉光，同时保留原有终端使用方式。本 fork 的实现、文档和开发脚本
包含经维护者授权的 AI 辅助工作；上游及第三方声明见 LICENSE 与 NOTICE.md。

## 主要功能

- 可恢复的 Foundation v1 单色显示效果与内置预设。
- Native、Native-tinted、Native-hybrid 色彩路径，配合 Tone/Bloom 调整。
- 全局设置、Profile 覆盖和逐项 Reset；设置页实时预览与预设管理。
- 可选终端风格 IME 候选浮层：微软拼音负责候选和提交，支持键盘分页、
  鼠标选词及系统 UI 回退。候选框与字形辉光可单独开启。
- 配色与校准图表工具，便于对照灰阶、代码色彩和细字可读性。

包含实验性 panel-v1 静态网格，默认关闭；专项 GPU/DPI 验收范围有限。

## 候选框使用

在全局或 Profile 的“外观”页开启终端风格输入法候选框；候选框和柔光默认
均关闭。保存后生效，开启候选浮层从下一次候选会话开始应用。也可使用 JSON：

```json
{
    "terminalEL.imeCandidateOverlay": true,
    "terminalEL.imeCandidateGlow": true
}
```

Profile 设置可覆盖全局，Reset 恢复继承。环境变量 `TERMINALEL_IME_OVERLAY`
精确值 `1`/`0` 会覆盖保存设置，日常使用应移除诊断覆盖。

## 已知范围

- 候选浮层已获维护者本地使用反馈；其他 IME、跨屏 DPI 和特殊布局的覆盖范围
  以实际验收记录为准。接口不支持、候选数据无效或空间不足时请求系统 UI。
- 候选字形辉光使用 Composition alpha-mask 阴影，与终端着色器的加法辉光
  并非逐像素一致；高对比度模式会关闭候选装饰辉光。
- 设置预览限同一进程。多编辑器预览采用最新的完整快照。
- 时间余辉不在当前实现范围内。
- 面向 Windows 10 2004（19041）或更高版本的 x64 环境。已完成本机 Release
  构建、安装及启动验证；尚未完成干净机器依赖验证，缺失运行库时可能需要
  Microsoft Visual C++ x64 Redistributable。
- 发行/安装器版本为 0.1.0，程序内部保留上游构建版本；包内
  `terminalEL-build.json` 记录源码提交与实际 EXE 版本。

## 安装与升级

运行 `TerminalEL-0.1.0-windows-x64-setup.exe`，默认安装到
`%LOCALAPPDATA%\Programs\TerminalEL`。升级前备份 settings 目录。
首版不自动注册为默认终端，也不自动更新。`wtel` 通过用户 App Paths 支持
运行对话框启动，不修改 PATH，不承诺作为命令行命令使用。
