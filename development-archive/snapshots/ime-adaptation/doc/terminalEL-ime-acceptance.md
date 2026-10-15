# IME overlay boundary acceptance record

Local AI-assisted preparation, 2026-10-15. This is an executable manual test plan,
not a claim that the application was run. Maintainer has accepted the normal
overlay, responsiveness, settings, mouse selection and glyph/outline glow locally.
All boundary cases below remain **not run** until results are recorded.

After the pointer-release hardening round the maintainer reported “没有问题”.
This is positive local acceptance; no individual case IDs/results were supplied,
so the detailed statuses below remain unreported rather than assumed passed.

## Record the environment

- Build/version and date:
- Windows version and Microsoft Pinyin mode:
- Monitor scaling (each monitor), font/size and window size:
- Effective global/Profile overlay and glow settings:
- Result format: case ID, pass/fail, actual behavior, screenshot/log if useful.

Use a fresh process without TERMINALEL_IME_OVERLAY for saved-settings tests.
Keep TERMINALEL_IME_PROBE unset during responsiveness checks. Enable it only for
a separate diagnostic run; it records metadata, not candidate strings or keys.

## Cases

| ID | Action | Expected result | Status |
| --- | --- | --- | --- |
| B01 | In two panes with different fonts/colors, compose in A, focus B, then compose in B. Repeat with tabs/windows. | No stale A list in B; B follows its own appearance; input goes to the focused terminal. | Not run |
| B02 | While composing, open Search/settings; then return. Move/detach or close the composing pane. | Overlay is removed on focus loss/teardown; no orphan glow, stale click or crash. A new composition works. | Not run |
| B03 | Compose near all four edges. Shrink the pane until a full page cannot fit; enlarge and start a new composition. | Full page above/below cursor or system fallback; no clipped page or duplicate UI. New composition can use overlay again. | Not run |
| B04 | With candidates visible, zoom font in/out and resize. Repeat with long wrapping candidates. | Text, glyph glow and hit targets remain aligned; oversized pages fall back. | Not run |
| B05 | Move a composing window between different-DPI monitors; test available 100/125/150/200% scaling. | Candidate position, text and glow follow the new layout without persistent offset. Record unavailable scales as not tested. | Not run |
| B06 | Save overlay off during composition, then enable again. Repeat using external settings reload. | Off requests system UI without losing composed text; on takes effect at the next candidate session. | Not run |
| B07 | Global on, profile off, profile Reset; save/reopen each. Repeat for glow. | Explicit profile wins; Reset restores inherited value; opening settings alone creates no override. | Not run |
| B08 | Toggle glow during composition; change palette/high contrast. | Sharp text remains readable; high contrast suppresses glow; restore normal mode and verify settings again. | Not run |
| B09 | Click first/middle/last rows on multiple pages; press then move outside or update the candidates before release. | Ordinary click commits once; canceled/stale click commits nothing; keyboard focus stays in terminal. | Not run |
| B10 | Hold left on a row and release right while left remains down. If supported, introduce a second pointer. | No commit on unrelated release. Only matching left release may select; canceled press may select nothing. | Not run |
| B11 | Double-click padding/footer, right-click and scroll over candidates in a mouse-reporting program. | No terminal word selection or shell mouse events escape; candidate rows still select normally. | Not run |
| B12 | Type long phrases, page repeatedly, Backspace to empty, Escape, number/Space/Enter selection; compare glow off/on with probe off. | No missing/duplicate commits, stale pages or noticeable new input lag. Record comparison rather than claiming a benchmark. | Not run |
| B13 | Switch IME/language, Pinyin compatibility mode and Win+. emoji. | Supported sessions work; unsupported UI keeps system behavior; returning to Pinyin works. | Not run |
| B14 | Save changes to monochrome core RGB/Hue/Saturation and GlowHue/GlowSaturation; compare GlowMix 0 and 1. Repeat with Profile override/Reset and terminal native mode. | Candidate text/ordinal/footer use adjusted monochrome core; glyph and outline glow use the mixed glow color; mix 0 matches core, mix 1 uses glow hue. Open candidates refresh after Save. High contrast retains readable appearance colors. | Not run |

## Completion

Keep failed cases with reproduction details. Do not infer a pass from static
checks. Once boundary results are acceptable, confirm the separate panel/calibration
status in `terminalEL-panel-calibration.md`, then choose release version and package
format. Build, packaging and publication are separate maintainer decisions.
