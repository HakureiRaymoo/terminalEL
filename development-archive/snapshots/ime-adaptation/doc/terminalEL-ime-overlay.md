# terminalEL IME candidate overlay — experimental v1

Local AI-assisted implementation. Default off. As of 2026-10-15 the maintainer has
accepted local overlay responsiveness, settings, mouse selection, outer glow and
candidate glyph glow/alignment. This is not exhaustive coverage of the acceptance
matrix below. The agent has not compiled or run the application.

## Enable after manually rebuilding

Current glyph bloom revision (locally accepted): replaced displaced text
copies with two Windows Composition DropShadows using TextBlock.GetAlphaMask,
behind the original sharp text. Near and wide Gaussian shadow bands follow the
TerminalEL core/tail design; existing TFELBloomIntensity (sqrt gain), BloomRadius
and BloomSoftness map to bounded opacity and DIP blur radii. This is alpha-blended
Composition bloom, not identical to TerminalEL's additive shader/threshold/tint
pipeline. No extra text copies, no animation. Candidate body clips glyph bloom;
outer bands remain separate. Inner edge bands increased to 0.28/0.12 opacity.
Verify that masks update after text/font changes, wrapping and DPI changes; compare
long-input responsiveness against glow-off. Settings apply after saving. Intensity
zero removes glyph bloom; outline decoration remains controlled by the glow switch.

The mask host and both shadow sprites follow the arranged Grid size using
RelativeSizeAdjustment, matching the stretched TextBlock rather than its intrinsic
DesiredSize. This corrected the reported glow compression toward the ordinal gap.
Sharp text remains above the non-hit-testable shadow host; the old offset text
copies are no longer used.

Optional visual stage (locally accepted): **Candidate window soft
glow**, JSON `terminalEL.imeCandidateGlow`, defaults false. Global/profile inheritance
and Reset match the overlay switch. Three static 2-DIP outline bands extend 6 DIPs
outside the candidate border, with opacity 0.04/0.08/0.16 and terminal foreground
color. These outer outlines are separate from the inner bands and glyph bloom.
High contrast disables all glow; insufficient edge clearance suppresses only
the outer bands. Decoration cannot receive pointer events and is
cleared with the candidate UI. This is a lightweight XAML approximation, not the
terminal EL shader pipeline. Verify light/dark palettes, high contrast, edges,
DPI, switching settings, and typing/click responsiveness with glow on/off.

Settings integration (locally accepted): use **Appearance →
Terminal-style IME candidates**, or the same switch in a profile's Appearance
page. Default false. Profile overrides and profiles.defaults take precedence over
the global value; the profile reset button removes the local override. Save to
apply. Enabling applies to the next candidate Begin; disabling an active overlay
requests system UI at its next timer refresh without terminating composition.

JSON key: `"terminalEL.imeCandidateOverlay": true` at the root (global) or in a
profile/profiles.defaults. This is independent of EL color presets.

Before testing the settings UI, remove `Env:TERMINALEL_IME_OVERLAY` from the launch
shell and start a fresh process. Exact values `1` and `0` remain diagnostic
overrides forcing on/off; an absent/invalid value uses saved settings. The override
is read once. Probe remains independently controlled by `TERMINALEL_IME_PROBE`.

Verify global on → inherited profile on → explicit profile off → Reset restores
inheritance, then save/restart. Also verify external settings reload during a
composition, and that enabling waits for the next candidate session. Existing
IME/edge/DPI matrix below still requires targeted manual acceptance.

For diagnostic environment-based startup instead:

```powershell
$env:TERMINALEL_IME_OVERLAY = '1'
$env:TERMINALEL_IME_PROBE = '1' # optional DebugView metadata
Start-Process -FilePath 'D:\OpenCodestuff\terminalEL\bin\x64\Debug\WindowsTerminal\WindowsTerminal.exe' -WorkingDirectory 'D:\OpenCodestuff\terminalEL\bin\x64\Debug\WindowsTerminal' -ArgumentList '-w new'
```

Variables are read once when TSF initializes. A new window alone does not prove
the process inherited them. Probe events: `subscribe`, `overlay-begin`,
`overlay-ready`, `overlay-system-fallback`. No candidate text or keys are logged.
To disable, remove `Env:TERMINALEL_IME_OVERLAY` in the launching shell and restart
the test process yourself. Remove `Env:TERMINALEL_IME_PROBE` for normal mode too.

## Contract

- Pane-local, non-focusable XAML list with terminal font family, cell-height-scaled
  type size and terminal background; selected row uses a 1-DIP core-colored
  outline with transparent fill. Border space is reserved on all rows, so moving
  selection does not shift text. Terminal SelectionBackground is not used.
- Candidate text, ordinals, footer and sharp outline use monochrome core RGB:
  PhosphorRed/Green/Blue, then Hue offset and Saturation multiplier, matching
  BackendD3D's core-color conversion and normalized bounds. Glyph shadows and
  inner/outer outline bands use GlowHue/GlowSaturation at the core's peak value,
  interpolated with the core by GlowMix. Mix zero therefore uses core color;
  mix one uses the independent glow color. These effective Profile settings apply
  even when terminal content uses a native color mode. No separate candidate
  palette or setting is added. High contrast retains system appearance foreground.
  Full-coverage foreground luminance uses monochrome Tone before coloring text.
  Bloom uses the same threshold/soft-knee and peak-envelope equations as a scalar
  approximation; near/wide shadows use render-pixel radius divided by DPI
  scale, with no fixed minimum expansion. Softness controls extraction, not radius.
  Shadow strength normalizes the scalar envelope by its 0.25 emission cap before
  applying near/wide opacity weights of 0.65/1.0, compensating for blurred alpha
  attenuation. The emission cap is not reapplied as an opacity cap. This mapping
  requires visual comparison with terminal text; exact intensity parity is not
  established by the static checks.
  The near band uses 0.75 of the radius and the wide band uses 1.20 of the radius:
  less tight edge reinforcement avoids an artificially bold-looking candidate.
  Text uses the renderer's resolved font face and weight, explicit normal style,
  and terminal em size including zoom. Independent XAML text scaling is disabled.
  Chinese font fallback and antialiasing can still differ between the two renderers.
  XAML rounds RGB to 8-bit; this does not reproduce the shader's per-pixel edge
  Tone, core protection or additive composite. Saved settings are
  picked up by candidate refresh; unsaved renderer-only previews are not promised.
- Current page/selection and current/total page count come from TSF. Row ordinals
  describe positions, not new key bindings. Keyboard paging and selection stay
  with the IME and existing composition path. No candidate text is sent to PTY.
- Mouse selection (locally accepted): left press/release
  on the same candidate row calls TSF SetSelection then Finalize. Rows do not take
  keyboard focus. A candidate revision and provider/document check reject stale
  clicks after typing, paging or focus changes. Pointer exit/cancel cancels a press.
  Padding/footer and wheel events are consumed; keyboard paging remains unchanged.
  No candidate strings are directly sent to the terminal.

Mouse acceptance: click first/middle/last rows on multiple pages; verify exactly
one commit, retained keyboard focus, and continued typing. Press then move outside
or change candidates before release: no stale selection. Check split panes,
right-click, double-click and mouse-reporting applications for accidental terminal
selection/input. Failed behavior calls should restore system UI.
- Below the cursor when it fits, otherwise above; clamped horizontally to the
  pane. Insufficient room returns to system UI instead of clipping a page.
- Only owned candidate elements exposing the behavior interface are eligible.
  Existing activation flags are retained; other TSF hosts do not opt in.
- A 16 ms UI-thread timer coalesces candidate Update notifications and waits up to
  250 ms for an initial empty/pending count. Once ready, candidate data is read
  only after an Update notification; idle ticks do not query strings/page indexes.
  Invalid metadata/strings or unsupported geometry cause immediate fallback.
  Cached-page layout checks track cursor/font/appearance at approximately 100 ms
  intervals without Update notifications. Rows are reused across text and selection
  changes; unchanged text/font/color properties are not reset. Overlay updates
  skip the probe's duplicate metadata/string queries. IMEs must send Update while
  their UI is suppressed; verify no stale page after this optimization.
- Page indexes must increase from zero; selection must be in the current page.
  Bounds: 4096 pages, 32 rows per page, 4096 UTF-16 code units per entry.
- End/focus loss/pane detach/close clears strings, stops the timer and removes
  the overlay. Provider and document checks prevent cross-pane presentation.
  Fallback calls `ITfUIElement::Show(TRUE)`; actual IME support needs acceptance.

## Maintainer acceptance

Record boundary results in [the acceptance matrix](terminalEL-ime-acceptance.md).
It includes matching-pointer/button releases and padding/footer double-clicks.

1. Default-off fresh process: normal system candidates and keyboard behavior.
2. Enabled Microsoft Pinyin: short/long phrases, arrows, all normal paging keys,
   first/last page, Backspace to empty, number/Space/Enter selection and Esc.
   Confirm exactly-once input and unchanged command ordering.
3. All cursor edges; shrink a pane to cause `overlay-system-fallback`. Enlarge and
   start a new composition to retry. Check for missing/double candidate windows.
4. Split panes with different fonts/schemes; switch pane/tab/window focus while
   composing, open Search/settings, detach/move/close the composing pane.
5. Resize, scroll, zoom fonts, reload appearance and change monitor DPI while
   composing. Check placement/readability and fallback.
6. Switch language, compatibility mode and Win+. emoji. Capture metadata for
   regressions; unsupported interfaces should keep system behavior.

Read-only checks cannot validate C++/WinRT, COM callbacks or actual IME behavior.
The existing `python tools/Test-TerminalELUI.py` now also checks both IME switches'
EN/ZH resource coverage, two-way binding and profile Reset target. Stabilization
adds double-tap consumption at the candidate border: manually double-click padding
and the page footer in a mouse-reporting application and confirm no terminal
selection or shell mouse event escapes. Candidate-row behavior is unchanged.
User build command (not executed by the agent):

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File "D:\OpenCodestuff\terminalEL\tools\Build-TerminalEL.ps1"
```
