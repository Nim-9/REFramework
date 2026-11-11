# GPTK 4 compatibility investigation

Status: experimental source fix, not a claim of game compatibility. No game has
been run with this change yet. Windows CI and WARP cannot validate D3DMetal.

## Source evidence

Audited Nim-9 branches master (431d98d9), backup/gptk-original (693f9611),
gptk4-compat (128bb9a8), queue-recovery-test (d6e1409a), gptk-upscaler
(0d678549), and gptk-pd-upscaler (1b074da2). Compared current upstream
praydog master f00d6bee and common ancestor 849c35a0. Upstream D3D12Hook.cpp
has no newer fix relative to that ancestor; its native wrapper scanning remains.
The upscaler trees have distinct SDK/rendering/TemporalUpscaler implementations;
the shared graphics changes are applied to both without replacing their features.

The previous Wine implementation scanned object memory, called AddRef/Release on
arbitrary candidates, inferred identity from reference counts, used an unvalidated
0x4c8 fallback, and applied an interface-pointer delta derived from a dummy queue
to unrelated real objects. Equal reference counts do not establish COM identity.
The diagnostic variants disabled some heuristics but returned before installing
Present/factory hooks if no offset was found. Waiting for VM and renderer frame 1
before hooking also misses early real swapchain creation. Factory interception
covered only CreateSwapChainForHwnd and declared its DESC1/output parameters as
DESC/IDXGISwapChain instead of DESC1/IDXGISwapChain1.

## Runtime evidence found locally

Log timestamps are recorded as written; their timezone is not independently known.
The basename re2_framework_log.txt is shared by all games and does not identify RE2.

| Game | Log timestamp | Recorded branch / commit | Observation | GPTK version |
|---|---|---|---|---|
| RE2R | 2026-11-11 12:46:20 | gptk4-compat / e28f88d8 | frame 1, dummy HWND swapchain succeeds, queue lookup fails, no ImGui init | unverified |
| RE3R | 2026-11-10 12:50:37 | gptk4-queue-recovery-test / d6e1409a | frame 1, direct scan offset 0, graphics setup aborts | unverified |
| RE4R | 2026-11-10 12:48:27 | gptk4-queue-recovery-test / d6e1409a | some integrity signatures fail; still reaches frame 1; direct scan offset 0 aborts graphics setup | unverified |
| RE9 | 2026-11-10 17:11:35 | gptk4-queue-recovery-test / f28658ca | frame 1, refcount match 0x28, delta -0x58; log ends just after two PointerHook installations; no logged ImGui initialization | unverified |
| RE4R historical | 2026-11-04 09:24:44 | gptk-upscaler / 0d678549 | reaches ImGui and REFramework initialized, later message-hook activity | unverified |

The first three logs were under the corresponding game folders in
/Volumes/Storage/Steam/steamapps/common; RE9 was under the macOS Steam library's
RESIDENT EVIL requiem BIOHAZARD requiem directory. Historical RE4 evidence was
found in an earlier local native-engine-output folder. Full logs are kept local,
not committed (they include machine paths and installed mods).

The available RE2 log is NOT a successful baseline. User-reported RE2 success is
not contradicted by a failed run of a different DLL, but cannot be compared as a
successful path without that run's log and runtime metadata. None of these logs
proves GPTK 4 Beta 3 or GPTK 3. notproton build files say CrossOver 26.3.0.39832;
backend records/launcher logs have different timestamps and cannot identify the
D3DMetal binary loaded by these sessions. RE9 also loaded custom pak mods, so it
is not an isolated clean baseline.

The concrete RE3/RE4 failure is the offset gate in graphics initialization.
The RE9 ending is evidence of the last recorded stage, NOT proof of a crash
location or of a queue-offset-only cause. A crash backtrace is still needed.
RE4 signature warnings need an executable-version-specific investigation if they
persist after graphics initialization works. No new integrity offsets/signatures
are guessed from those warnings.

## Implementation

* Install Wine DXGI factory capture on the startup thread immediately after the
  logger/build identity starts, before unpacking, integrity work and engine waits.
* Capture pDevice via documented CreateSwapChain, CreateSwapChainForHwnd,
  CreateSwapChainForCoreWindow and CreateSwapChainForComposition entry points.
  D3D12 passes a command queue here; D3D11 passes a device and is rejected by QI.
* Validate DIRECT queue type and queue/swapchain device IUnknown identity. Store
  the association using DXGI SetPrivateDataInterface, which owns its COM reference.
  Retrieve through GetPrivateData, retain with ComPtr, and validate against the
  device used for rendering. No arbitrary pointer is invoked on the Wine path.
* Install Present independently of queue offset discovery on Wine. Re-arm Present
  on a newly captured real swapchain if hooks already exist. Missing associations
  log an explicit error and suppress rendering callbacks, leaving original Present
  running. ImGui never receives an unvalidated Wine queue.
* Preserve enumerated-adapter Wine dummy device creation and Wine virtual-memory
  handling. Do not restore on-disk D3D12 export bytes under Wine. Skip C++ RTTI
  probes on Wine COM objects. Scope-bound dummy cleanup covers failure paths.
* Preserve native Windows discovery and all upscaler/VR/game features. Wine/GPTK3
  uses the same API-based capture; regression testing is mandatory.
* Add engine-integrity stage markers and ImGui device/queue/buffer/format logs.
* Extend existing Dev Release workflow with real Windows WARP queue tests:
  missing association, device input rejection, compute queue rejection, interface
  alias retrieval, exact queue identity, wrong device rejection, cleared data.

API contracts: [CreateSwapChain](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-createswapchain),
[SetPrivateDataInterface](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiobject-setprivatedatainterface),
[GetPrivateData](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiobject-getprivatedata).
Factory vtable indices are SDK COM interface slots, not game/object memory offsets.

## Remaining limits and exact runtime tests

Factory hooks currently cover the factory vtable discovered from the active DXGI
module. An interposer with a separate factory vtable, an already-created swapchain,
or a wrapper that does not forward private data may evade capture. Late injection
cannot safely reconstruct a queue; use a cold launch. Present1 and ResizeBuffers1
queue replacement are not covered by this implementation. A missing Present after
capture needs a call trace/vtable comparison, not a new offset. Frame generation
and multiple active swapchains require separate verification. The ID3D12Device4
requirement of the existing renderer is retained; failure is not bypassed.

For EACH of RE2R/RE3R/RE4R/RE9, EACH standard/upscaler DLL, run:

1. Record game executable version/hash, game patch/Steam build ID, macOS/hardware,
   CrossOver/notproton build, graphics backend, exact GPTK package Beta revision,
   and SHA-256/path of the D3DMetal binaries actually loaded. Record DLL hash,
   logged branch and commit. Current installed files alone cannot prove old runs.
2. Preserve old logs before launching. Cold launch with third-party scripts,
   plugins, pak mods, ReShade and frame generation disabled using reversible
   configuration/renames, retaining backups. Keep the currently working RE2 DLL.
3. Confirm ordered evidence: early factory hooks; real creation association_hr=0;
   VM and renderer ready; Present validated (queue/device); both ImGui backends;
   REFramework initialized; visible responsive menu; load save; play >=10 minutes.
   DLL injection, successful hooks or compilation alone are not a pass.
4. Open/close menu, change resolution/window mode, alt-tab repeatedly, reload save,
   quit/relaunch three times. Check queue/device changes and reset diagnostics.
5. Re-enable existing scripts/plugins and supported upscalers individually. Confirm
   actual upscaled rendering, image stability, menu rendering, no GPU hangs; test
   frame generation separately. Retain failed logs and crash/system reports.
6. Repeat RE2R baseline and relevant games on GPTK 3, and Windows/DX12. RE2R DX11
   (where available) is a separate control and cannot validate the DX12 fix.
7. For any failure attach the complete REFramework log plus launcher output and
   crash backtrace from that SAME session, with module list and runtime hashes.

Still needed: a successful RE2R log with matched runtime provenance; same-session
GPTK 4 Beta 3 provenance for all four existing failure logs; fresh logs from both
new builds for all four games; RE9 crash/backtrace and clean-mod control; RE4
executable identity to assess integrity pattern misses. Existing logs for all four
were found; they must not be relabeled as verified Beta 3 sessions.
