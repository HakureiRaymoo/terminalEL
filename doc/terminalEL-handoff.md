# terminalEL handoff — 2026-10-12

## Next goal (maintainer approved)

### Maintainer-approved IME branch snapshot

Maintainer confirms the current version is basically usable and authorizes a new
branch, commit and push. Target: `feature/terminal-ime-adaptation`. README now
identifies the terminal IME adaptation, default-off switches, accompanying Native/
panel/settings work, AI assistance and validation limits. Preserve current work
as a source snapshot; no package or new build requested. Historical pending visual
notes below are superseded by this overall local acceptance, not itemized test data.

### Candidate outer glyph halo adjustment pending visual retest

Maintainer finds near-stroke glow sufficient and requests more visible outer
glyph glow. Keep near radius/opacity at 0.75/0.65; increase only wide radius from
1.0 to 1.20 and opacity weight from 0.90 to 1.0. Text, colors, selection outline
and window decoration are unchanged. No build performed; verify outer reach and
separation between adjacent glyphs in a manual screenshot comparison.

### Candidate font/style consistency pending screenshot retest

Maintainer screenshots show heavier/brighter candidate stroke edges than terminal
body. Candidate TextBlocks now use the renderer's resolved face and weight rather
than requested FontFace plus inherited XAML weight. Normal style is explicit;
XAML accessibility text scaling is disabled for these already-terminal-sized
glyphs to avoid independent scaling on top of terminal zoom. Near shadow changes
from 0.5 radius/1.0 weight to 0.75 radius/0.65 weight; wide 1.0/0.90 is retained
to keep surrounding glow without reinforcing strokes as much. Color equations
unchanged. Chinese fallback, rasterizer AA and per-pixel shader core protection
still differ from XAML; do not claim exact font rasterization parity. No build.

### Candidate bloom additional strength adjustment

Maintainer screenshot requests another increase. Raised near/wide shadow opacity
weights from 0.85/0.65 to 1.0/0.90 (about 18%/38% per-band increase). Radius and
sharp text are unchanged; Bloom zero still removes glyph shadows. Pending visual
retest; no build performed.

### Candidate bloom strength correction pending visual acceptance

Maintainer requests candidate glow as strong as terminal text. Previous mapping
used the shader's 0.25 output-emission cap directly as shadow opacity and then
split it across bands, underpowering the already blurred alpha tails. Normalize
the scalar envelope by its 0.25 cap and use 0.85/0.65 near/wide opacity weights.
Bloom intensity/threshold/soft knee still control the response; zero extraction
or intensity gives zero glyph shadow. Radius, core colors, font and outline
selection remain as implemented. This is stronger Composition compensation,
not measured pixel parity with the renderer. Manual side-by-side retest required.

### Candidate font size matches terminal em size (pending visual acceptance)

Maintainer requests candidate size similar to terminal body. Replaced cell-height
times 0.75 with requested terminal font point size converted to DIPs (96/72).
ControlCore exposes a C++ helper using _desiredFont, so zoom is included and custom
cell height does not distort glyph size. All candidate text shares this size.
No compilation; compare body/candidates at normal and zoomed sizes and mixed DPI.

### Candidate selection outline and restrained bloom pending visual acceptance

Maintainer screenshot shows unreadable bright selection fill and requests bloom
closer to the terminal text above it. Removed SelectionBackground from candidate
rows: selected row now has a 1-DIP core-colored outline with transparent fill.
All rows reserve the border, subtracting its size from padding to avoid movement.
Candidate core now applies monochrome luminance/Tone to full-coverage foreground.
Bloom uses the monochrome soft-knee extraction and 0.25 peak envelope as a scalar
proxy; radius converts render pixels to DIPs, with half/full-radius shadow bands.
Removed fixed 2/6-DIP expansion and the incorrect softness-to-radius mapping.
Composition alpha shadows remain an approximation, not per-pixel shader parity.
Manual build/visual comparison remains required; frozen renderer is unchanged.

### Candidate monochrome color integration pending acceptance

Maintainer requests candidate colors to follow monochrome TerminalEL, including
independent glow color. TermControl now mirrors normalized core RGB -> Hue /
Saturation and GlowHue / GlowSaturation / GlowMix calculation from BackendD3D.
Text/ordinals/footer/sharp outline use core; glyph and inner/outer shadows use
mixed glow. Uses effective Profile settings irrespective of native color mode;
high contrast keeps appearance foreground and suppresses glow. XAML colors are
8-bit, not a claim of shader-equivalent tone/emission/compositing. No build run.
Retest saved core hue/saturation, glow mix 0/1, inherited Profile overrides and
reset, especially while candidates are already open. Release prep waits for this.

### Stabilization feedback accepted; release preparation (2026-10-15)

After pointer-release hardening and the boundary-plan round, maintainer reports
“没有问题 继续吧”. Record positive local acceptance; no itemized B01–B13 results
were supplied, so preserve Not run per-case status rather than infer full coverage.
Added `terminalEL-release.md` with Release/x64 layout, portable isolation, package
staging and actual-archive verification workflow; release notes are a separate
draft. Version, package format and panel-v1 inclusion await maintainer decision.
No build, staging, archive, commit or publication has occurred in this round.

### Candidate glyph glow accepted; stabilization (2026-10-15)

Maintainer confirms candidate glyph glow is adjusted and requests the next step.
This supersedes the pending visual status below: overlay responsiveness, settings,
mouse selection, outer glow and glyph alignment have positive local acceptance.
Historical entries below describe earlier implementations, not the current design.
Current bloom uses two alpha-mask Composition shadows sized to the arranged row.

Next phase is stabilization and local release preparation. Added candidate-border
double-tap consumption for padding/footer, and extended the existing UI checker
to cover both IME switches' EN/ZH resources, two-way bindings and profile Reset.
The pointer change still needs a manual padding/footer double-click check.
Use the boundary matrix in terminalEL-ime-overlay.md for remaining IME/DPI/focus
coverage; local acceptance does not imply that entire matrix was exercised.
Before packaging, agree version/package format and confirm panel/calibration
acceptance. No build or publication performed by the agent.

Boundary review follow-up: candidate presses now retain PointerId; release must
match that pointer and have left button up before consuming the pending selection.
This prevents unrelated pointer/right releases from finalizing a held left click.
Manual verification remains pending. `terminalEL-ime-acceptance.md` records 13
boundary cases with explicit Not run status and reproducible actions/expectations.

### Glyph bloom alignment fix pending visual retest

Maintainer screenshot shows bloom displaced/compressed toward the number/text
gap. Code sized the shadow sprite from TextBlock.DesiredSize although the text
and mask stretch to the arranged row width. Replaced intrinsic-size sampling with
relative sizing on both container and sprite (same Grid cell as sharp text).
Composition now follows arranged size without an extra layout/update loop. This
is a suspected mask scaling cause, not yet a visually verified fix. Test short/
long rows, selected row, page changes, wrapping, font zoom and monitor DPI.

### Candidate bloom redesign pending build/acceptance

Maintainer finds prior text readable but glow too weak, requests TerminalEL-like
intensity/spread. Replaced offset text with two Composition DropShadow glyph-mask
bands behind sharp text; reads existing BloomIntensity/Radius/Softness. Near/outer
tail architecture follows native composite design, but alpha compositing is not
shader-equivalent. Stronger inset borders, clipped glyph bloom, no frozen shader
changes. Must verify GetAlphaMask refresh, text/font/wrapping/DPI and performance.

### Outer glow accepted; inner/text halo extension pending acceptance

Maintainer sees outer glow and reports no other issues, requests inner/text glow.
Added two inset bands and four static subpixel-offset text copies beneath sharp
text, controlled by existing default-off IMECandidateGlow. Lazy-created/reused,
removed when disabled, non-hit-testable, accessibility Raw. High contrast disables
all glow; pane edges disable outer decoration only. No compilation. Verify dense
glyph clarity, wrapping, DPI and typing performance before accepting this stage.

### Mouse accepted; optional candidate outline glow implemented

Maintainer reports no issues with mouse selection and requested continuation.
Added default-false `terminalEL.imeCandidateGlow` global/Profile switches with
EN/ZH resources, inherited fallback and Reset. XAML uses three non-hit-testable
static outline bands (6 DIP extent), colored from terminal foreground. No shader
changes or blur surfaces. Suppressed at edges and in high contrast, cleared on
candidate teardown. Pending maintainer build/visual/performance acceptance.

### Settings accepted; mouse selection implemented pending build/acceptance

Maintainer reports no issues with settings integration and requested continuation.
Added non-focusable row pointer selection through TSF SetSelection/Finalize.
Snapshots carry absolute page start and revision; Update/Begin/End invalidate
stale clicks. Press/release, provider and focused document checks gate commits.
Pointer exit/cancel cancels selection. Padding/footer/wheel events are consumed.
Keyboard input remains on existing TSF path. No agent build. Verify exactly-once
commit, no focus theft, stale press rejection and shell mouse isolation before
moving to optional visual effects. Older keyboard-only notes are historical.

### Settings integration implemented, pending maintainer build/acceptance

Added `terminalEL.imeCandidateOverlay` (false) to global/profile model, adapter,
control settings and EN/ZH Appearance switches. Profile inheritance resolves
before global fallback; Reset clears local override. Save/reload updates effective
settings. TSF XAML host now subscribes independently of opt-in to support runtime
enable; disabled owned candidates are explicitly allowed system UI. Provider
checks effective setting at Begin and refresh: enable waits for next session,
disable requests system UI without terminating composition. Environment exact
0/1 still overrides settings (remove it in a fresh process for UI acceptance).
Foundation, UI resource/source checks, ProjectGraph and diff whitespace passed.
No compilation performed. See `terminalEL-ime-overlay.md` for inheritance and
mid-composition reload checks. Next gate is user build and boundary acceptance;
mouse selection and optional glow remain later stages.

### Current acceptance: optimized overlay looks normal locally

After the performance optimization, the maintainer reports “现在看起来没有问题了”.
Record this as positive local visual/responsiveness feedback, not an exhaustive
acceptance of every IME, DPI, paging or fallback scenario. Keep the feature opt-in
and keyboard-only. No further implementation change requested in this feedback.
Earlier blocked/pending notes below are historical; no benchmark was supplied.

### Fifth capture: overlay visible; performance optimization subsequently retested

`ALICES-MAGICBOO5.log` includes PID 10900 overlay-ready and composition callbacks.
Maintainer sees terminal-style UI but reports lag, particularly with longer input.
Changed Update to dirty-mark/coalesce on a 16 ms UI timer; ready idle ticks no
longer read candidate strings/page indexes. Cached layout checks run about every
100 ms. XAML rows survive text/selection changes, unchanged properties are not
reset, and overlay Update skips duplicate probe metadata reads. No compile run.
Retest long continuous input, selection-only movement, paging and fallback;
compare with probe disabled to separate DebugView overhead. No measured speedup
claimed. Earlier 50 ms full-page polling described below is superseded.

### Stage 2 acceptance blocked — third capture

`ALICES-MAGICBOO3.log`, PID 2044: subscribe succeeds, focus/unfocus only;
no candidate Begin/Update/End. Maintainer reports no IME UI and no Chinese input.
Root cause is not established. Existing capture lacks composition/profile events;
absence of candidate events alone does not prove composition never started.
Added probe-only overlay enabled/disabled, post-SetFocus identity verification,
active keyboard profile type/language/HRESULT, and composition lifecycle events.
No input text/keys logged. Next: same-build fresh-process overlay-off versus on
comparison, explicitly selecting Microsoft Pinyin; if unresolved, maintainer
rebuild with new diagnostics. Do not treat stage 2 as accepted or fixed.

Fourth capture: `ALICES-MAGICBOO4.log`, PID 12932, follows the requested
overlay-off comparison. Maintainer reports normal behavior. Candidate Begin,
`allow-system-ui`, owned candidate/behavior interfaces and End recur normally.
No overlay/configuration/focus-verification events from the newly added
diagnostics appear, consistent with the prior binary. This confirms a working
system-UI baseline, not overlay recovery or a proven root cause. Next: maintainer
build with diagnostics, fresh-process overlay-on capture with Microsoft Pinyin
explicitly selected; compare profile/focus/composition and takeover events.

### IME candidate overlay — experimental stage 2 implemented locally

`TERMINALEL_IME_OVERLAY=1` enables a pane-local keyboard-only XAML candidate
overlay. See `doc/terminalEL-ime-overlay.md`. TSF retains the element/provider
pair, validates the current-page indexes and strings, and requests system UI
on failure. A 50 ms timer bounds the initial pending/empty wait to 250 ms and
tracks geometry/appearance. End/focus loss/detach/close clears the list and timer.
No new activation flags, settings schema, PTY path or shader changes. Optional
probe logging is independent. Maintainer build and runtime acceptance remain.
Read-only checks passed: Foundation (including XAML XML), ProjectGraph and
`git diff --check`. These do not validate C++/WinRT or COM/runtime behavior.

### IME candidate investigation — stage 1 accepted

Maintainer approved the TSF-first assessment. Added an observation-only candidate
probe enabled by process environment `TERMINALEL_IME_PROBE=1`, read once when the
XAML TermControl TSF handle initializes. OutputDebugString diagnostics report
interfaces, document ownership, lifecycle and metadata HRESULTs, not input text.
TSF activation flags are unchanged. Initial user capture (PID 7404) confirmed
candidate/behavior/owned interfaces and successful candidate metadata/string reads,
but system UI was missing: Begin received FALSE and the probe preserved it.
Fixed to explicitly return TRUE for candidates in our active document, with
`allow-system-ui` logging. This correction was subsequently built/visually verified.
Foundation guard and diff whitespace check passed. Capture steps and interpretation:
`doc/terminalEL-ime-probe.md`. Next: maintainer build and Microsoft Pinyin capture;
review evidence before controlled takeover and a terminal-style XAML overlay.
No Update while system UI is shown is allowed by TSF and is not a failure.
Follow-up accepted: `ALICES-MAGICBOO2.log` (PID 22264) shows subscription and
`allow-system-ui`; maintainer confirms the system candidate window and tested
interactions work normally. The corrected probe has therefore been built and
locally exercised by the maintainer. Next development step: opt-in terminal-style
XAML candidate overlay, retaining system-UI fallback and existing activation flags.

### Panel/calibration round implemented locally (2026-10-12)

Assessment and implementation details: `doc/terminalEL-panel-calibration.md`.
Optional `terminalEL.panelGrid` (panel-v1), default 0, range 0..0.15, now has
global/Profile slider + NumberBox, inheritance/Reset and live preview. Native
composition attenuates a static neutral 4x4 render-pixel grid; no new passes or
textures. Monochrome ignores it; Foundation guard and frozen shaders unchanged.
Display adjustment is independent of color presets (not captured or applied).
Preview is now **18 floats**: mode remains index 16, grid is index 17. CPU/GPU
constants remain 80 bytes (settings.glow.z -> native phosphorColor.w).
`Show-TerminalELColors.ps1 -Page Calibration` adds black/white/gray/channel steps,
fine text, uniform patch and instructions; default Colors preserves prior chart.
No build performed. Maintainer build + GUI/GPU acceptance is the next step; then
decide whether to keep/tune the optional grid before release preparation.
New non-compiling check: `python tools/Test-TerminalELPanel.py`.
Checks passed this round: Foundation, Native, UI, ProjectGraph, Panel and
`git diff --check`. Calibration and All chart modes executed successfully in pwsh.
These checks do not validate C++/WinRT, FXC, XAML activation or GPU output.

Skip temporal persistence. Next assess/design optional panel characteristics and
display calibration, then prepare the first official release after acceptance.
Do not implement arbitrary default visual changes. Favor readability, color
distinction and low overhead; proposed panel features should be optional and off
by default. A calibration page can reuse color/gray/code samples to compare modes.
Explain significant UX/rendering choices before implementing them.

## Constraints

- Repo: D:\OpenCodestuff\terminalEL; fork origin HakureiRaymoo/terminalEL.
- Read AGENTS.md. AI assistance/publication permission applies to this fork.
- Do not compile unless explicitly requested; user builds and visually tests.
- Do not clean bin/obj/generated headers or terminate Terminal processes.
- Preserve all workspace edits. No subagents unless explicitly requested.
- Foundation v1 must remain restorable: frozen five shaders and approved preset.
  Do not update guard hashes to accommodate changes.
- First baseline commit: e5ebf2089, "Add terminalEL Foundation v1 rendering and settings".
  Agent push failed with connection reset; latest local status reports main aligned
  with origin/main. Check remote status again before publication.
- Native/hybrid/tone work remains uncommitted. Current instruction is preparation
  for a future official release, not to publish unfinished new features now.

## Current visual modes

terminalEL.colorMode: monochrome (0), native (1), native-tinted (2), native-hybrid (3).
Default remains monochrome. Mode is carried in preview array index 16, after the
original 16 numeric values. Global and Profile UI, inheritance Reset, real-time
preview and preset create/update/import/export all carry mode.

- monochrome: frozen Foundation v1 algorithm. Approved brightness 2, core RGB
  (1,.82,.04), glow hue 18/saturation 1/mix 1. Full preset in foundation doc.
- native: preserves source hues, source-colored glow.
- native-tinted: uses glow hue/saturation/mix to tint halo.
- native-hybrid: near-gray pixels get monochrome core and independent glow color
  (default yellow/orange-red); actual colored pixels keep their own colors/halos.
  Glow switch/mix is ignored in hybrid. Classification occurs BEFORE Tone and
  per sample BEFORE downsampling. Saturation <= .04 fully neutral, >= .12 colored,
  smooth transition. Composed pixels, not text semantics: gray backgrounds and
  low-saturation colors can be mapped; ClearType fringes need visual attention.

Native renderer reuses passes with RGBA16F half-resolution bloom, uses new shaders
tfel_native_extract_ps.hlsl / blur / composite and tfel_native_tone.hlsli.
No extra passes/targets for hybrid. Native runs 4 postprocess draws with bloom,
1 without; retained scalar emission texture is unused in native.

Native composition was strengthened at user request: sqrt weak-tail lift,
amplitude 3, .25 soft cap and core protection, scalar RGB-ratio-preserving envelope.
Tone now affects BOTH sharp frame and extraction (brightness/contrast/black/gamma),
using unpremultiplied RGB peak and restoring alpha. Neutral Tone is exact identity.
User tested brightness 2 and called the result beautiful.

FXC X4000/X3129 happened with early returns in nativeTone. Fixed with initialized
outputColor and single return plus safe alpha/peak denominators. User confirmed
successful result. Avoid reintroducing multiple early returns in that helper.

All five old v1 shader files unchanged. TFELSettings and GPU constants remain
80 bytes: mode uses settings.glow.y; native hybrid uses glowColor.w=-1 sentinel.
Native presets renderer=native-el-v1; old presets renderer=tfel-v1, version=1.

## Settings and lifecycle

Preset selection alone never applies. Save persists clone; Reset discards edits.
Only real numeric/mode property names forward preview; metadata-only preset
operations no longer steal preview priority. Shared bounds validate create,
update, import, load and serialization. Invalid/unknown library entries remain
in raw JSON; editable entries use original storageIndex.

Multi-editor previews still use latest whole snapshot, not field-level merging.
Successful settings reload rebuilds all editors and may replace unsaved changes.
See review doc. Performance currently acceptable: defer async font loading.

## Clean local palette and command highlighting

User asked to restore EL Amber Code after experimentation. Local settings file:
C:\Users\zhang\AppData\Local\Microsoft\Windows Terminal\settings.json

At inspection it still contained EL Amber Code with modified brightYellow/selection.
Restored these to #FFD16B / #173C59; all other fields match canonical file
doc/EL-Amber-Code.json. profiles.defaults already selects EL Amber Code.
Preserved unrelated Campbell (copy) scheme and all user settings.
Later added an optional variant `EL Amber Code - Campbell Cursor`: all palette
colors unchanged, cursorColor and selectionBackground both #FFFFFF (Campbell).
Canonical variant: doc/EL-Amber-Code-Campbell-Cursor.json. Not auto-selected.
Observed global TFEL: native-hybrid, brightness=2, contrast=1.05, gamma=1.05,
blackLevel=0, userPresets="[]". Other parameters inherit defaults.

PowerShell 7 profile:
C:\Users\zhang\Documents\PowerShell\Microsoft.PowerShell_profile.ps1
Contains PSReadLine Command True Color ESC[38;2;232;245;90m (#E8F55A lemon yellow).
Other functions steam/viaclash preserved. New shells load it; existing shells:
Set-PSReadLineOption -Colors @{ Command = "$([char]27)[38;2;232;245;90m" }
Read color using (Get-PSReadLineOption).CommandColor, NOT .Colors['Command'].
This is separate from Terminal's brightYellow slot.

tools/Show-TerminalELColors.ps1 shows ANSI16, backgrounds, gray/near-gray,
RGB, 256 colors, simulated Python/TS/C++/PowerShell/JSON and diff diagnostics.
Executed successfully under pwsh; not an actual editor theme installer.

## Commands and documentation

User build:
pwsh -NoProfile -ExecutionPolicy Bypass -File "D:\OpenCodestuff\terminalEL\tools\Build-TerminalEL.ps1" -Run

Non-compiling checks:
- python tools/Test-TerminalELFoundation.py
- python tools/Test-TerminalELNative.py (arithmetic/source model, not GPU execution)
- python tools/Test-TerminalELUI.py
- python tools/Test-TerminalELProjectGraph.py
- python tools/Test-TerminalELColorScheme.py "<settings.json path>"
- git diff --check

Read doc/terminalEL-foundation-v1.md, terminalEL-native-color.md,
terminalEL-review-2026-10-12.md. terminalEL-status.md includes historical sections.
Before release: agree version/packaging, review changes, exclude artifacts/logs/
dependencies/local settings, retain AI disclosure, perform user build and GUI
acceptance, then explicitly authorized commit/push/release steps.
