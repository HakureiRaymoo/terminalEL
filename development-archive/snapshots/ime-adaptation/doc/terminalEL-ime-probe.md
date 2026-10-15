# terminalEL IME candidate capability probe

Local AI-assisted development. With the overlay variable absent, observe existing TSF UI elements.
The opt-in stage 2 contract is in `terminalEL-ime-overlay.md`.
No build or runtime acceptance has been performed by the agent.

## Contract

- Off by default. Exact process environment value `TERMINALEL_IME_PROBE=1`
  enables the probe when the XAML terminal host initializes its shared TSF handle.
- Does not add UI-less activation flags, call UIElement::Show, select candidates, change
  paging, submit input, or change settings. Begin explicitly returns `pbShow=TRUE`
  for candidates belonging to our active document; other elements retain their
  incoming decision. This requests the system UI rather than drawing a replacement.
- `ITfUIElementSink` is subscribed on the existing thread manager, independently
  of the existing context sinks. Subscription failure does not fail TSF startup.
- Uninitialize unadvises the sink before deactivating TSF. No COM element objects
  are retained across callbacks. End logs its ID without querying an expired UI.
- Only the XAML TermControl opts in. Shared TSF consumers conhost/HwndTerminal do
  not opt in. The environment variable must be inherited by the actual new app
  process, not merely by a shell running inside an already-open Terminal.
- Diagnostics use `OutputDebugStringW`, prefix `[terminalEL IME probe]`, with PID,
  thread ID, element ID, focus/composition status, candidate/behavior/reading
  interface availability, document ownership, counts, selection and HRESULTs.
  No candidate strings, composition strings, or keystrokes are logged; no log
  files are created by the app. Debugger/capture software controls log storage.
- Candidate content readiness is checked only on Update, for our document with
  a current provider. GetString is probed for the selected candidate and freed
  immediately without logging its contents. Begin data may not be ready.

## Maintainer verification after building

1. Open Visual Studio's native debugger Output window, or Sysinternals DebugView
   with Win32 capture enabled. Filter for `terminalEL IME probe`. Start capturing
   before opening the test process. DebugView may not receive output from a process
   already attached to a debugger; use that debugger's output instead.
2. From a separate PowerShell process, set `$env:TERMINALEL_IME_PROBE = '1'` and
   launch the **newly built local WindowsTerminal.exe directly**. Use the actual
   executable path from your build, not the installed `wt` alias. Ensure startup
   did not delegate to a pre-existing process: the `subscribe` PID is the evidence.
   A new window alone does not prove a new process/environment.
3. Focus a terminal pane. Expect `subscribe hr=0x00000000` and `focus`. Type a
   known test phrase with Microsoft Pinyin. Observe the normal system candidate
   UI; test number selection, Space, Enter, Esc, Backspace and candidate paging.
4. Switch panes/tabs/windows while composing; focus Search and a Settings text
   field; return to the terminal. Test input-language switching and Win+. emoji.
   No missing/duplicate characters, hidden candidate UI or reordered Enter should
   result. Compare with a newly launched process with the variable absent.
5. Record Windows build, IME name/new-vs-compatibility mode, app PID, operation
   sequence and captured probe lines. Remove the environment variable from the
   launching shell after the test; it does not disable an already-running probe:
   `Remove-Item Env:TERMINALEL_IME_PROBE`.

## Interpreting results and next gate

### First local capture: ALICES-MAGICBOO.log

PID 7404 subscribed successfully. Microsoft Pinyin exposed candidate=1,
behavior=1, owned=1, successful count/page/selection/GetString calls and selection
changes. The user reported no visible candidate window. Every Begin received
incomingShow=0: the initial probe mistakenly preserved FALSE instead of explicitly
allowing its own system candidate UI. Fixed by returning TRUE for our active
document and logging `allow-system-ui`. This fix needs a maintainer rebuild and
visual retest. Updates may stop after this correction, which is permitted by TSF.
The first capture proves interface/data availability, not nonintrusive behavior,
full page-index correctness or working mouse selection.

### Second local capture and maintainer acceptance

`ALICES-MAGICBOO2.log`, PID 22264: subscription succeeded and owned candidate
Begin callbacks logged `allow-system-ui`. Updates ceased and Begin/End pairs
were observed. The maintainer confirmed that the system candidate window appeared
normally and the tested interactions worked without issues. This accepts the
corrected observation probe for the tested local configuration, not all IMEs.
Together with the first capture, this supports proceeding to an explicitly
enabled terminal-style candidate overlay experiment without changing activation
flags. Full page-index retrieval, custom mouse behavior and fallback from custom
UI still require implementation and acceptance.

| Observation | Meaning / next action |
| --- | --- |
| No subscribe | Wrong process/build/environment, TSF not initialized yet, or capture not working; not evidence of an unsupported IME |
| Subscribe failure | Capture HRESULT; keep normal input path |
| Begin with candidate=1 | Candidate UI interface is exposed under current activation flags |
| owned=1 | Candidate DocumentMgr COM identity matches this terminal TSF document |
| behavior=1 | Candidate behavior interface exists; selection/finalization still untested |
| Only Begin/End, no Update | Allowed when system UI is shown; not evidence of failure |
| E_PENDING (0x8000000A) | Probe deliberately did not query content, not necessarily a TSF error |
| selection HRESULT S_FALSE | No selection; numeric selection output must not be interpreted |
| pages HRESULT S_FALSE | Zero-capacity size query may report insufficient capacity; this is not necessarily a broken list |
| docHr fails during Begin | May be premature; retry evidence from Update, do not conclude permanent mismatch |
| Successful owned Update/GetString | Metadata and a selected candidate can be read; does not prove complete paging or click support |

The next stage is a controlled, explicitly enabled takeover experiment only after
reviewing this capture. It must verify Update delivery, page indexes, candidate
strings, fallback and focus ownership when system UI is suppressed. If normal
activation supplies no usable notifications, separately assess UI-less activation
in an isolated experiment: `ActivateEx` flags are additive and can affect other
text controls on the thread. Do not silently enable the restrictive flag globally.

After that gate, prefer an unfocusable XAML candidate overlay using terminal font,
colors and selection styling. Preserve existing composition/PTY ordering. Send
mouse selections through the supported TSF behavior interface, never directly as
terminal input. EL shader integration is a later presentation step.

## References inspected

- https://learn.microsoft.com/en-us/windows/win32/tsf/uiless-mode-overview
- https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfuielementsink-beginuielement
- https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfthreadmgrex-activateex
- https://github.com/libsdl-org/SDL/blob/SDL2/src/video/windows/SDL_windowskeyboard.c
- https://wiki.libsdl.org/SDL3/SDL_HINT_IME_IMPLEMENTED_UI

This implementation follows the existing project COM lifetime/style and official
interfaces; no SDL source was copied or new dependency added.
