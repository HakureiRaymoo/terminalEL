# Native Color + EL Glow — first implementation

Panel/calibration follow-up: see `terminalEL-panel-calibration.md`. Optional grid
is applied after native composition, off by default. Preview now carries 18 floats
(mode index 16, grid index 17). Grid is separate from the color preset contract.

Baseline commit: `e5ebf2089` (Foundation v1 and settings UI). The initial push
failed with a connection reset; the maintainer can run `git push origin main`.
Native work is local and uncommitted pending manual build and visual acceptance.

## Modes

### Hybrid addition: neutral phosphor

`native-hybrid` (mode index 3) maps near-neutral source pixels to the configured
monochrome core color and independently maps their emission to glow hue/saturation
(Foundation defaults: yellow core and orange-red glow). Colored pixels retain
their own core/emission colors. glowMix and the independent-glow switch are ignored
in this mode so neutral emission is not accidentally changed to yellow. Core hue,
saturation and phosphor RGB apply to the neutral core; Tone applies to both classes.
Presets retain the existing native-el-v1 identifier with this explicit mode string.

Decision: use the existing extraction/composition passes, with no new render targets
or draw calls. Classification uses source RGB before Tone, per full-resolution sample
before the 2x2 downsample: saturation <= 0.04 maps fully, >= 0.12 is unchanged,
with smoothstep between. Core and emission get separate target colors; emission
is recolored before blur, never classified after neighboring colors have mixed.
Incremental work is a few scalar operations and a lerp per sample. Actual GPU
overhead is not measured; no frame-rate claim is made.

Alternatives: CPU foreground remapping alone loses neutral-vs-original-yellow
identity before postprocessing and cannot reliably assign different halos. A
semantic glyph mask or separate emission render target would improve text/background
separation and ClearType handling but adds plumbing/bandwidth/draw overhead. Deferred
until screenshots show that postprocess classification is insufficient.

Limitations: classification sees the composed frame, not text semantics. Gray
backgrounds/selections can be recolored; very desaturated intentional colors can
enter the transition; ClearType fringes or colored backgrounds can change a pixel's
classification. Against black, proportional grayscale antialiasing retains the
classification. With neutral Tone, black remains black. Reduced contrast can raise
black to gray under the existing Tone curve and then map it to phosphor color.

Acceptance: compare white/gray, saturated primaries, genuine yellow, pale colors,
adjacent red/gray, dark backgrounds, ClearType/grayscale AA, selection and emoji.
Check neutral orange halos vs genuine yellow halos, intensity zero, brightness 1/2,
Profile Reset, Save/cancel and hybrid preset export/import. To reproduce the approved
palette, Apply Foundation v1 first, then select hybrid (retains brightness 2).

`terminalEL.colorMode` supports:

- `monochrome`: existing frozen TFEL v1 path; still the default.
- `native`: original composed frame plus source-colored glow.
- `native-tinted`: original frame plus glow blended toward the configured hue
  and saturation using glowMix. Zero mix follows source color; one uses the tint.

Global and Profile Appearance expose the three modes. Profile mode supports
inheritance and Reset. Preview transport appends mode at index 16 to the existing
16 numeric values; it now carries 17 floats. Save and cancel use the existing
session lifecycle, with its documented whole-snapshot multi-editor limitations.

## Renderer

Native uses three new shaders: half-resolution RGB extraction, separable RGB
blur, and composition onto the tone-adjusted original frame. Extraction uses maximum
RGB channel as brightness to avoid suppressing saturated blue/red. Bloom threshold,
softness, radius and intensity apply. Brightness, contrast, black level and gamma
now use a shared tone function in extraction and composition. The curve acts on
the unpremultiplied RGB peak and scales channels together to retain hue, then
restores alpha premultiplication. Brightness 2 doubles values below clipping;
already-saturated peaks remain limited to 1. Phosphor RGB and core hue/saturation
remain monochrome-only. Tone affects backgrounds and selections as well as text.

The source is the composed terminal frame, not a glyph-only mask. Bright colored
backgrounds, reverse video, cursor and selections can emit glow. This is not yet
text-only emission. No linear-light/sRGB conversion is introduced in this draft.

Two half-size RGBA16F bloom textures replace the two R16 textures in native mode;
their storage is four times larger. Full-size scalar emission is currently retained
but not drawn in native mode to minimize resource-lifecycle changes. Native uses
four postprocess draws with bloom, one with zero intensity; v1 keeps five/two.
Switching between scalar/color paths recreates targets. Switching native tint mode
only changes constants. Custom shader/Retro retains priority, and TFEL remains D3D-only.

Zero bloom intensity returns the tone-adjusted RGBA value; neutral tone
(brightness/contrast/gamma 1, black level 0) preserves original RGBA exactly. Following the
maintainer's screenshot comparison, native composition now lifts weak tails with
the monochrome-style square-root envelope (amplitude 3, peak limit 0.25) and
protects bright cores with the same 0.25-scale envelope. Nonlinear shaping acts
on the halo's scalar peak; RGB ratios are retained rather than independently
compressing channels. Scalar headroom prevents clipping. Nonzero glow raises
alpha as needed to retain premultiplied output.
This strengthens the halo without recoloring the original frame, but does not
promise identical perceived strength: monochrome brightness/emission and source
colors differ. Recheck colored backgrounds, dark counters, transparency and
small nonzero intensities against zero after building.
Transparency, ClearType edges, images and light themes require visual validation;
this is not a declaration of complete transparency compatibility.

The old five shader files, monochrome CPU color resolution and presets are retained.
Applying Foundation v1 selects monochrome again. Unknown mode strings fall back
to monochrome at the renderer boundary.

## Presets

Version stays 1. Monochrome exports use `renderer: tfel-v1`; native and tinted
exports use `renderer: native-el-v1` with the corresponding colorMode string.
Import requires matching renderer and mode. Each user snapshot stores its mode;
Create, Update, Apply, Export and matching status include it. All 16 numeric fields
remain required even when a mode ignores some fields. Existing built-ins are unchanged.

## Manual acceptance

1. Build with the existing Build-TerminalEL.ps1 entry point. Agent does not compile.
2. Select native in global Appearance. Check ANSI colors, syntax highlighting,
   Git diff, cursor, selection, reverse video and a bright background.
3. With neutral tone and Bloom intensity zero, colors and alpha should match the
   original frame. Set brightness to 2: unsaturated text should brighten, with
   its RGB ratios preserved. Re-enable bloom to verify emission also responds.
   Test contrast, black level and gamma separately, including transparent backgrounds.
4. Compare source-colored glow with tinted glow at mix 0/0.5/1, especially red,
   green and blue text. Tune threshold/radius and examine dark glyph counters.
5. Override Profile mode, Reset it, Save/reopen, and close unsaved settings.
   Check cross-window live mode changes within one process.
6. Create/export/import/update a native preset. Apply Foundation v1 afterward:
   original single-color effect and its matching status must return.
7. Repeatedly switch modes, resize to odd dimensions, minimize/restore, test
   high DPI and transparent/image backgrounds; inspect for stale or blank pixels.

Non-compiling guards passing does not validate shader compilation, XAML activation,
GPU behavior or visual quality.
