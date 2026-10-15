# Optional panel characteristics and visual calibration — panel-v1

Local AI-assisted implementation, 2026-10-12. No build or GPU acceptance yet.

## Assessment and decision

Priorities: readable small text, distinct syntax colors, stable motion, low cost,
and an exactly restorable Foundation v1. Temporal persistence is out of scope.

| Candidate | Benefit | Cost / drawback | Decision |
| --- | --- | --- | --- |
| Static neutral pixel grid | Subtle panel structure without RGB tint | Can beat against glyphs/DPI or shimmer when scrolling | Implement optional, bounded panel-v1 |
| RGB subpixel mask | Strong physical-display appearance | Hue shifts, ClearType interaction, monitor layout assumptions | Defer |
| Scanlines | Familiar display texture | Not intrinsic to an EL flat panel; horizontal strokes lose contrast | Defer |
| Grain / temporal noise | Less uniform appearance | Visual distraction, redraw demand, potential flicker | Defer |
| Vignette / nonuniformity | Panel edge character | Uneven code readability, window-position dependence | Defer |
| New calibration XAML page | Discoverability | Ordinary XAML swatches bypass terminal shaders; a real terminal preview needs additional lifecycle/UI work | Use actual terminal chart first |
| ICC/HDR or automatic monitor calibration | Colorimetric accuracy | Requires display-specific characterization beyond current encoded-RGB renderer | Not claimed |

## Implemented contract

`terminalEL.panelGrid`: float, default **0 (off)**, range **0..0.15**. Available in
global and Profile Appearance with slider/numeric entry; Profile supports inherited
values and Reset. JSON follows the existing global -> profiles.defaults -> Profile
resolution. Renderer clamps nonfinite/out-of-range values. Live preview appends
grid strength at index 17 (18 floats total), preserving the prior indices.

Only native/native-tinted/native-hybrid use this setting. The five frozen Foundation
shaders, preset numbers and default mode remain intact. Setting mode to monochrome
therefore restores the v1 renderer even if the grid value is nonzero.

`panelV1` is an opt-in output modifier layered over native-el-v1, not a replacement
color algorithm. It uses 4x4 **render-pixel** cells, anchored at terminal surface
origin. A one-pixel row/column separator is attenuated once at intersections:
`rgb *= 1 - strength`; other pixels are unchanged. A period contains 7 separator
pixels out of 16, so mean attenuation on a uniform patch is `7 * strength / 16`
(maximum 6.5625%). No compensating brightness increase or RGB-channel mask.

Applied after sharp/halo composition, including zero-bloom and zero-halo exits.
It leaves alpha and emission extraction unchanged; it cannot create light, alter
RGB ratios, or violate valid premultiplied input bounds. On transparent surfaces
the desktop contribution is not modulated, so the perceived grid can be weaker.
Strength zero returns the existing result without arithmetic on its RGB.

No additional draw, texture, history buffer, timer or shader compilation target.
CPU settings and GPU constants stay 80 bytes: settings.glow.z transports strength;
native phosphorColor.w carries it to the composite shader. This field remains zero
for monochrome. Actual GPU overhead has not been measured.

Grid is intentionally a **display adjustment separate from color presets**:
Create/Update/Import/Export and preset match status describe color/Tone/Bloom only.
Applying a preset does not overwrite this display setting. Exporting a color preset
does not export grid strength; record it separately when reproducing a display.
Future incompatible grid geometry requires a new explicit panel version/path.

## Calibration workflow

From the repository in an existing terminalEL window:

```powershell
pwsh -NoProfile -File .\tools\Show-TerminalELColors.ps1 -Page Calibration
pwsh -NoProfile -File .\tools\Show-TerminalELColors.ps1 -Page Colors
# Or both sequentially:
pwsh -NoProfile -File .\tools\Show-TerminalELColors.ps1 -Page All
```

The default remains the existing Colors chart. The script only prints; it does not
write settings, launch a terminal, change presets, or install an editor theme.
Charts scroll normally; use at least ~100 columns for unwrapped comparison rows.

1. Record current settings (including Profile overrides), monitor, Windows scaling,
   font/size, AA mode, opacity, theme and window size. Keep these fixed for A/B.
2. In Appearance, start with native, brightness/contrast/gamma 1, black level 0,
   Bloom intensity 0, grid 0. Use a separate terminal window in the same process
   to see live previews. Do not confuse stale whole-editor snapshots with A/B.
3. Observe black/white steps and fine dark text. Adjust one Tone control at a time;
   increased brightness can clip highlight steps. The existing gamma control is
   an exponent (greater than 1 darkens midtones), not a monitor gamma measurement.
   Visibility of the smallest steps is not guaranteed and is not a pass/fail target.
4. Reintroduce Bloom, checking dark counters, adjacent colors and saturated channels.
   Compare all modes; hybrid intentionally remaps neutral ramps, so evaluate Tone
   and channel clipping in native first. Monochrome intentionally collapses hues.
5. Optionally compare grid 0 / .05 / .10. Inspect thin strokes, uniform patch,
   selection, scrolling, window resize and 100/125/150/200% scaling. Leave it off
   if it creates stripes, shimmer or reduced legibility. Fixed render-pixel pitch
   is not a physical panel-pitch or DPI calibration.
6. Save accepts editor adjustments; closing without Save reverts that preview
   session under the existing lifecycle. Preset export saves color settings only.

## Manual acceptance after maintainer build

- Opening either page must not create overrides. Numeric input, sliders and live
  preview agree; test empty, invalid, out-of-range values and focus loss.
- Global value, profiles.defaults override, Profile override and Profile Reset;
  Save/reopen, Reset editor, close without Save, two terminal windows.
- Grid zero matches preceding native behavior at Bloom 0 and nonzero; positive
  grid works with no halo and with halo. RGB colors stay distinct; transparent
  backgrounds, cursor, selection, emoji and images remain usable.
- Apply Foundation v1 with grid .15: approved monochrome result returns. Switch
  back to native: the independent grid value returns. Preset library operations
  and exports must not silently change it.
- Check odd surface sizes, minimize/restore, AA modes, DPI and scrolling. GPU
  performance and appearance remain unverified until the maintainer checks them.
