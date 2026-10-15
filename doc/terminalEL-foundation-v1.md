# Foundation v1 — approved visual baseline

Native Color development now has a separate path documented in
`terminalEL-native-color.md`. The v1 contract below remains unchanged; the older
monochrome-only feature descriptions refer to the approved baseline. Current
preview transport adds a mode field after the 16 numeric values, and native
presets use the distinct `native-el-v1` renderer identifier.

The maintainer approved the current visual result on 2026-10-11. Preserve this
look as an immutable built-in preset, not as an alias for future defaults.

The inspected local unpackaged settings contain global `terminalEL.brightness: 2`
and no profile-level TFEL overrides. Therefore Foundation v1 uses brightness 2,
not the source default of 1. Unsaved previews or a different running settings
location cannot be inferred from this file. Fonts, terminal color schemes,
antialiasing, DPI, and monitor settings also affect the result.

## Parameter snapshot

```json
{
  "terminalEL.brightness": 2.0,
  "terminalEL.contrast": 1.0,
  "terminalEL.blackLevel": 0.0,
  "terminalEL.gamma": 1.0,
  "terminalEL.phosphorRed": 1.0,
  "terminalEL.phosphorGreen": 0.82,
  "terminalEL.phosphorBlue": 0.04,
  "terminalEL.bloomIntensity": 0.6,
  "terminalEL.bloomThreshold": 0.3,
  "terminalEL.bloomRadius": 8.0,
  "terminalEL.bloomSoftness": 0.1,
  "terminalEL.hue": 0.0,
  "terminalEL.saturation": 1.0,
  "terminalEL.glowHue": 18.0,
  "terminalEL.glowSaturation": 1.0,
  "terminalEL.glowMix": 1.0,
  "terminalEL.colorMode": "monochrome"
}
```

The executable UI preset is `TFELBuiltinPresets[0]` in
`src/cascadia/TerminalSettingsEditor/TFELPresets.h`. The Apply button explicitly
writes all parameters to the settings clone; Save persists them. Applying a
global preset does not erase profile overrides. Merely opening the page or
selecting a name must not change settings.

## Renderer compatibility contract

Foundation v1 uses the current five-pass scalar-emission pipeline, R32 emission,
half-resolution R16 bloom, normalized nine-tap separable blur, threshold/knee,
and the protected-core composite. Composite constants are core scale 0.25,
halo lift 0.50, amplitude 3.0, maximum 0.25. Core and halo are independently
colored on composition; halo is capped by remaining scalar emission headroom.

Future visual algorithms must retain the v1 path and introduce explicit version
selection before changing these shaders, color conversion, formats, sampling,
or tone equations. Parameter preservation alone is insufficient. Do not update
the baseline guard just to make an incompatible change pass.

## Current development stage

- Added Foundation v1, Classic amber v1, Soft gold v1, Green phosphor v1.
- Added global hue/saturation, independent glow toggle, glow hue/saturation/mix.
- Current renderer and source defaults remain unchanged in this stage.
- Custom presets can now be created from current global values. A trimmed,
  case-insensitively unique name is required. Save persists the library; Reset
  or closing without Save discards unsaved library changes along with settings.
- The library is stored as a versioned JSON array encoded in the global string
  setting `terminalEL.userPresets`. This reuses the existing settings clone,
  serialization and Save lifecycle. Export writes an ordinary JSON object with
  `version`, `renderer`, `name`, and named `settings` fields to a chosen file.
  Unknown future-version entries are retained when adding presets. Malformed
  entries do not prevent opening Appearance; an invalid library JSON blocks
  creation rather than silently overwriting the existing data.
- Export uses the selected snapshot, not the current sliders. To export new
  adjustments, first create or update a custom preset. Profile overrides are not captured by
  this global editor.
- Import accepts exported v1 monochrome snapshots only. Every numeric field is
  required, finite, and within the renderer's supported range. It adds a library
  entry without applying it. Enter an optional new name before Import to resolve
  name collisions, including when importing a built-in preset as a user copy.
- Rename and Delete operate only on user presets, and do not change the currently
  applied parameters. Save commits the change; Reset discards all unsaved edits.
  Original storage indexes are retained so malformed/future entries skipped by
  the UI cannot make Delete remove the wrong entry. Other entries remain intact.
- Profile Appearance now exposes core hue/saturation, independent glow, glow
  hue/saturation/mix. The five numeric color controls include precise input and
  per-setting Reset. The switch and mix both reset TFELGlowMix; inherited mix may
  be nonzero. Existing Profile Tone/Bloom sliders remain available.
- Global TFEL labels and application-defined messages now use en-US and zh-CN
  resources. Built-in preset names remain stable, including Foundation v1.
- All 13 visible global numeric parameters have NumberBoxes alongside sliders.
  Enter or focus loss commits input through the existing preview path. Bounds
  match the sliders; non-finite input is not written to settings. One-way binding
  and user-focus-gated writeback prevent initialization from changing settings.
- Separate status compares all 16 global values and monochrome mode against all
  presets (absolute tolerance 0.00001), listing matches or adjusted/custom. It
  does not describe Profile overrides or whether the renderer is active.
- Update selected custom preset captures global values at its original storageIndex.
  Built-ins stay read-only. Unknown metadata, settings keys and other array entries
  are retained. Save/Reset behavior is unchanged.
- No compilation or GUI launch was performed by the agent; manually verify
  opening Appearance, Apply/Save/cancel, and the new bindings after building.

## UI acceptance for the 2026-10-12 round

1. Open Appearance in Simplified Chinese and English; opening alone must not
   alter parameters. Check narrow-window layout and accessibility labels.
2. Apply Foundation v1; verify matching status and the approved visual effect.
   Selecting another preset without Apply must leave both unchanged.
3. Type brightness 1.234, hue -17 and radius 12.5. Test Enter and Tab/click-away,
   slider synchronization, cross-window preview, empty/invalid/out-of-range text,
   and moving focus between two number boxes.
4. Create a custom preset, adjust values, update it, export and import under a new
   name. Updating a built-in must report read-only. Verify matching status after
   rename/delete/import and with duplicate snapshots under different names.
5. Save and reopen to verify persistence; Reset and closing without Save must
   restore parameters and the library. With future entries preceding a valid
   custom entry, updating it must preserve those entries and unknown metadata.

## Profile color UI acceptance

The maintainer accepted the preceding global UI round before this Profile work.
This new round still requires a manual build and GUI check.

1. Open an unmodified Profile's Appearance: colors must display inherited values
   without creating local overrides. Check the same in profiles.defaults.
2. Change hue or glow mix by slider and numeric input (Enter and focus loss).
   Verify the embedded preview and running terminals with that Profile update;
   a different Profile without a shared edited parent should retain its colors.
3. Override glow mix to zero while the parent uses a positive value. Verify the
   switch is off, then Reset from either switch or mix: the inherited value and
   switch state should return, and both Reset buttons should become disabled.
4. Test global fallback and a profiles.defaults override. Reset must remove the
   local key rather than copy the inherited value into the Profile's JSON.
5. Save/reopen preserves overrides; closing unsaved settings reverts previews.
   Verify empty/non-finite/out-of-range numeric input and keyboard focus after Reset.
