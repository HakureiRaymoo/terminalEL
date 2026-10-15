# TFEL settings lifecycle and performance review

## Follow-up implementation

After maintainer approval, the two narrow fixes were implemented locally:

- MainPage now forwards only the 16 numeric preview properties through an exact
  allowlist, for both global and Profile notifications. Preset metadata, computed
  match status and the redundant independent-glow notification cannot publish a
  stale snapshot. Save and Reset retain their explicit lifecycle notifications.
- Preset Create/Update/serialization share the same numeric bounds as JSON
  Import/library loading. JSON is validated before float narrowing. Create also
  rejects non-monochrome settings instead of silently exporting monochrome.
  Invalid library entries are skipped in the UI but retained in the raw JSON,
  and original storageIndex values continue to identify editable entries.

The initial findings below describe the pre-fix inspection. Multi-editor snapshot
policy, reload handling, dispatcher behavior and font loading are unchanged.
Manual verification: reproduce finding 1 and confirm rename no longer changes
the active preview; verify actual sliders and Apply still do. For finding 5,
verify out-of-range Create/Update/Import fail without mutation and an invalid
entry before a valid custom entry survives editing that valid entry.

Scope: source inspection only. No compilation, GUI launch, settings mutation,
runtime tracing, or measured latency. Existing maintainer feedback is that current
settings performance is acceptable. No application code was changed in this review.

## Recommendation

Defer asynchronous font loading and broad settings-page optimization. Prioritize
the small preview-notification correctness issue below, then manually exercise
two-editor lifecycle scenarios before choosing a larger multi-editor policy.

## Findings

### 1. Preset metadata notifications can take over the preview (high confidence)

`TerminalSettingsEditor/MainPage.cpp:666-674` forwards every property beginning
with `TFEL` to the preview publisher. `GlobalAppearanceViewModel.cpp` raises
`TFELCurrentPresetStatus` after create/import/update/rename/delete, even when
render parameters have not changed. `SettingsPaneContent.cpp:57-75` publishes all
profiles, and `TerminalApp/TFELPreview.h:31-41,114-124` gives the newest snapshot
priority for each profile it contains.

Reproduction to verify in GUI: open editors A and B; create a custom preset in A;
change brightness in B; return to A and rename that preset without Apply. A's old
parameter snapshot can replace B's preview. This does not write settings to disk,
but violates the intended separation between library operations and applying effects.

Suggested narrow fix: forward only actual rendering-parameter notifications,
excluding computed status/library metadata and redundant toggle notifications.
This requires no renderer or preset-format change.

### 2. Multi-editor previews are whole snapshots, not merged edits (confirmed design)

Every publication includes all profiles. If A changes profile P and then B changes
profile Q, B's older copy of P can also win. Reset publishes a baseline snapshot
with new priority; it does not simply remove that editor's preview session.
Closing an unsaved session removes it and reveals the remaining latest session.

Whether to keep this last-editor-snapshot model, isolate previews per edited
profile, or introduce finer-grained merging is a product decision. Do not silently
introduce merging: global inheritance and stale edits make it substantially more
complex than a notification filter.

### 3. Reload replaces all open editing copies (confirmed code path)

`TerminalApp/AppLogic.cpp:412-413` clears preview sessions on successful reload.
`TerminalPage.cpp:4089-4109` refreshes tabs; `SettingsPaneContent.cpp:78-82` updates
the editor; `TerminalSettingsEditor/MainPage.cpp:207-274` replaces its source and
clone, rebuilds profile VMs, and re-navigates. No dirty-copy preservation or merge
is visible in that path. Saving one page can therefore replace another page's
unsaved edits when its successful reload is distributed.

The replacement is the existing settings-editor lifecycle, not a newly introduced
TFEL-only merge rule. Decide whether independent concurrent editing is a supported
workflow before changing it.

### 4. Save/reload handoff and dispatcher shutdown deserve targeted validation

`TFELPreview::End` retains a saved snapshot until reload, avoiding immediate
rollback when a saved page closes. `Reloaded` clears sessions before the downstream
settings refresh. `Update` can recreate any session ID and has no reload generation
check. These warrant stress tests around Save followed immediately by edit/close
or Reset; source inspection alone does not prove a visible race in normal dispatch.

`TFELPreview.h:129-147` sets `lastValues` before TryRunAsync is confirmed accepted
and does not observe its Boolean completion. A rejected dispatch can make an
identical retry look redundant. Dead targets are removed by Unregister; queued
callbacks check an active token. Actual shutdown behavior remains unmeasured.

### 5. Preset validation has an edge-case inconsistency

Import and Update validate supported ranges; Create checks only finiteness;
library loading checks finite numeric fields but not their ranges. Manually edited
out-of-range settings can therefore produce a created/exported preset that Import
rejects. Normal bounded UI use is unaffected. A shared validation helper is a
reasonable small follow-up, separate from page performance work.

## Performance assessment

| Path | Source evidence | Assessment |
| --- | --- | --- |
| Settings open | MainPage copies settings, constructs several VMs, and builds all profile VMs | Work scales with configuration size; no measured bottleneck |
| First font-list access | ProfileViewModel.cpp:237-326 synchronously enumerates/sorts system fonts and caches static observable vectors | Potential first-use pause; defer async redesign while acceptable |
| Profile Appearance | Creates a preview TermControl; subsequent changes use a 100 ms trailing debounce | Embedded preview can lag during continuous dragging by design |
| Live terminal preview | SettingsPaneContent coalesces at 33 ms, resolves 16 values for all profiles; TFELPreview skips unchanged target values | Existing throttling is useful; no evidence to justify rewriting it |
| Preset status | Each parameter notification compares every preset's 16 values; Apply produces multiple notifications | Cost scales with library size; currently unmeasured |
| Reset/reload | Rebuilds settings clone/VMs and re-navigates | Distinguish this cost from initial page navigation before optimizing |

The font cache uses shared static state and single-threaded observable collections.
Any future background or multiple-UI-thread design must explicitly address cache
ownership, publication, and notifications rather than moving the current method
to a worker unchanged.

## Focused manual assessment

1. With two settings pages, reproduce metadata-only rename taking preview priority.
2. Edit different profiles in A/B; document whole-snapshot takeover and close fallback.
3. Leave unsaved changes in A, Save B, and inspect A after reload.
4. Save then immediately edit/Reset/close; verify eventual render matches saved JSON.
5. Close a preview target while dragging in another window; check errors and surviving targets.

If performance becomes bothersome, measure cold settings open, first Profile
Appearance, repeated Appearance navigation, and Reset separately. Record profile,
font and preset counts, build type, and repeated timings (median and slow tail).
Until such data exists, do not claim millisecond latency or an expected speedup.
