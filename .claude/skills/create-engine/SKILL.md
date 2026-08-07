---
name: create-engine
description: Create a new from-scratch engine .mr script for engine-sim (real bore/stroke/redline/firing order, correct DSL structure) and validate it by building and running the engine-sim-check tool against it. Use when asked to add, build, or create a new engine for engine-sim, port a real engine into the sim, or write a .mr file from scratch.
---

# Creating a new engine-sim engine

Engine-sim engines are `.mr` scripts (a custom Piranha-based DSL, not C++). Get the
DSL right by pattern-matching against existing files, not by guessing syntax.

## 1. Read reference files first

Read these in full before writing anything — they show the complete, proven
structure end to end:

- `assets/engines/atg-video-2/k20a_turbo.mr` — inline-4, turbo, full structure
  (wires / camshaft-builder / head / crankshaft / pistons / intake / exhaust /
  ignition / vehicle / transmission / `public node main`).
- `assets/engines/soli_dstate/toyota/corolla_2zrfae_cvt.mr` — inline-4 NA, CVT
  transmission, simpler (no turbo).
- `assets/engines/bmw/S55_dct.mr` + `assets/engines/bmw/S55_zf8hp.mr` — shows
  the "base engine in file A, second transmission variant in file B that
  imports file A" pattern for an engine offered with two gearboxes.
- `assets/engines/bmw/S54_amt.mr` — `automated_manual` transmission type
  (single-clutch robotized manual, e.g. SMG/F1-style paddle boxes).
- `es/objects/objects.mr` — the full node/parameter reference. Grep this for
  anything you're unsure of (transmission `type` strings and shift params are
  documented around line 672-720; turbocharger/intake/exhaust/camshaft
  parameters are all here too).
- `es/actions/actions.mr` — `add_turbocharger`, `add_exhaust_system`,
  `add_cylinder_bank`, etc.

Every top-level file ends with `public node main { set_engine(...);
set_vehicle(...); set_transmission(...); }` so it's directly openable via the
app's File > Open (no manifest/registry to update).

## 2. Gotchas that aren't obvious from the reference files

- **Node names can't start with a digit.** `private node 2jzgte_na { ... }` is
  a parse error. Use a name like `jz_na`, matching how
  `assets/engines/chevrolet/454_truck.mr` uses `bb454`, not `454`.
- **Twin-turbo**: two `turbocharger` nodes can share one `intake` node (their
  compressor outputs merge into one shared plenum, matching a real
  parallel-twin-turbo inline-6's actual manifold topology). See
  `assets/engines/soli_dstate/toyota/2jzgte.mr` or
  `assets/engines/soli_dstate/nissan/rb26dett.mr` for the full pattern: each
  turbo's `intake:` parameter points at the *same* intake, but each has its
  own `add_exhaust_system()` call to its own bank's exhaust (a contiguous
  front-half/rear-half cylinder split, not alternating).
- **Inline-6 crank throws pair up (1-6, 2-5, 3-4 share the same physical
  angle)** — verify your `rod_journal` angles land on the same value mod 360°
  for paired cylinders even if you write them as larger cumulative numbers for
  firing-order readability (see the comment block in `2jzgte.mr`'s engine
  node for a worked example).
- **V8/V10 journal sharing**: read an existing V8/V10 file
  (`assets/engines/angethegreat/atg-video-2/08_ferrari_f136_v8.mr` for a
  flat-plane V8, `assets/engines/angethegreat/atg-video-2/10_lfa_v10.mr` for a
  V10) for how two `cylinder_bank`s at an `angle` share rod journals across
  banks for a given crank type (flat-plane vs cross-plane matters for journal
  spacing).
- **Transmission `type` strings**: `"manual"`, `"automatic"` (torque
  converter), `"dual_clutch"` (DCT), `"automated_manual"` (single-clutch
  robotized manual — SMG, F1 paddle boxes, Aisin ASG), `"cvt"`. Match the real
  car's actual gearbox category, not just "auto vs manual".
- **Real-world specs**: use actual bore/stroke/rod length/compression/redline/
  firing order for the engine you're building; where you're not fully
  confident in an exact number (rod length, exact compression ratio, flow
  bench numbers, boost curve shape), make a labeled, plausible approximation
  — same spirit as the `// 80.5mm x 88.3mm, 1798cc -- real 2ZR-FAE dimensions`
  comment style already used throughout this repo. Flow bench numbers and
  boost curves don't need to be dyno-sheet accurate; dimensions, redline, and
  firing order should be.

## 3. Where files go

`assets/engines/soli_dstate/<manufacturer>/<engine_name>.mr` — lowercase
manufacturer folder name, matching the existing `soli_dstate/` engines
(`toyota/`, `nissan/`, `bmw/`, `honda/`, `ferrari/`, `volkswagen/`, `lexus/`,
`ford/`, `dodge/`, `vespa/`). Don't touch `assets/engines/angethegreat/` (the
older, separate engine set) or any loose top-level engine files.

If an engine has multiple transmission variants (e.g. manual + DCT), use the
S55 pattern: `<engine>_mt.mr` defines the full engine as a `public node`
(e.g. `public node foo_na`) plus the manual transmission as `main`;
`<engine>_dct.mr` imports the `_mt.mr` file, reuses that same engine node,
and defines a different transmission as its own `main`. Keep both files flat
in the same manufacturer folder (no `mt/`/`dct/` subfolders — that breaks the
sibling `import "..."` between them).

## 4. Validate with engine-sim-check

Once the `.mr` file is written, build (if not already built) and run the
`engine-sim-check` tool against it — it loads the file exactly like the real
app (no window/rendering) and runs four checks: does it start, does it reach
redline in neutral, a WOT dyno pull, and a full-load FPS benchmark.

Build (from repo root, Windows + VS Build Tools):

```
$env:VSROOT = "Z:\visual studio build tools 2026\product"
$env:PATH = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\WinFlexBison.win_flex_bison_Microsoft.Winget.Source_8wekyb3d8bbwe;$env:PATH"
$env:PATH = "C:\Program Files (x86)\Microsoft Visual Studio\Installer;$env:PATH"
$env:PATH = "$env:VSROOT\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;$env:PATH"
cmd /c "`"$env:VSROOT\Common7\Tools\VsDevCmd.bat`" -arch=x64 && cmake --build z:\vscode\engine-sim\build --config Release --target engine-sim-check"
```

Run:

```
build/engine-sim-check.exe assets/engines/soli_dstate/<manufacturer>/<engine_name>.mr
```

Read the output:

- **1) Starts?** should PASS with a plausible idle RPM (a few hundred to
  ~1500rpm depending on engine type). FAIL/stalled at ~0rpm after starter
  release usually means a script problem worth checking (wrong firing order
  vs cam/ignition wiring, an intake/exhaust parameter at zero that shouldn't
  be) rather than a tool problem — the tool itself already handles ignition
  enable and the starter/speed-control sequencing correctly.
- **2) Hits redline in neutral?** should PASS, reaching close to (or slightly
  over) the configured `redline`, in well under 2 seconds for anything
  sporty. If it gets stuck far below redline, suspect a script issue (e.g. an
  `idle_flow_rate`/`intake_flow_rate` too restrictive, or a redline number
  that's actually in the wrong units in the script).
- **3) Dyno run** should produce a smooth, physically sane torque/HP curve
  (compare peak numbers against the real engine's known specs — they won't
  match exactly, but should be in the right ballpark, e.g. within ~20-30%).
  A curve that's all zeros, or that spikes absurdly and then craters, means
  something is off in the engine's tune, not the tool.
- **4) Full load + FPS** just needs a reasonable sustainable-FPS number
  (tens to low hundreds); this mostly validates that impulse response
  loading and the audio pipeline didn't choke on anything in the script.

If a check fails, re-read the `.mr` file for the specific parameter that
would cause that symptom before assuming the tool is wrong — the tool has
already been validated against multiple known-good engines and correctly
reproduces realistic dyno curves for them.

`engine-sim-check` takes a path as a CLI arg, or shows a native file-open
dialog if run with no arguments (results still print to the console either
way; it only pauses for a keypress in the no-argument/dialog case).
