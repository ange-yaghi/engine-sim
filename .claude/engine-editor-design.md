# Visual Engine Editor — Design Document

## 1. Goal

Let a user build and tune an engine (crankshaft, cylinder banks, heads, cams, intake/exhaust,
turbo, ignition, fuel, vehicle, transmission) through GUI panels inside `engine-sim-app`,
instead of hand-writing a `.mr` script, while staying fully interoperable with the existing
`.mr` pipeline: an engine built in the editor must be a normal `.mr` file that opens, edits,
and shares exactly like the hand-written assets in `assets/engines/`.

Non-goals for this document: it does not propose a new save format, a new physics model, or a
web/cross-platform UI. It describes what needs to be added *inside the existing C++ app and its
`delta-studio`/piranha stack* to make visual editing possible.

## 2. What already exists (and why it constrains the design)

### 2.1 Script → runtime pipeline

`.mr` files are piranha node-graph scripts. `es_script::Compiler` (`scripting/src/compiler.cpp`)
compiles and executes them; each engine-sim concept (`crankshaft`, `cylinder_bank`,
`turbocharger`, ...) has a matching `*_node.h` in `scripting/include/` — e.g.
`CrankshaftNode` (`scripting/include/crankshaft_node.h:16-68`). Every node follows the same
shape:

- `registerInputs()` calls `addInput("script_name", &m_parameters.Field)`, binding a script
  identifier to a field on a `Parameters` struct (e.g. `Crankshaft::Parameters`,
  `Turbocharger::Parameters`).
- `generate(...)` copies `m_parameters` into the real runtime object's `Parameters` struct and
  calls `object->initialize(params)`, registering the mapping in an `EngineContext`
  (`scripting/include/engine_context.h`) so sibling nodes can look each other up (e.g. a
  `TurbochargerNode` finding the `Intake` a script pointed it at).
- `EngineNode::buildEngine()` (`scripting/include/engine_node.h:26-149`) is the top-level
  assembly: it walks crankshafts → cylinder banks → cylinders (piston, rod, rod journal, intake,
  exhaust) → cylinder head (flow curves, valvetrain, cam) → turbochargers → ignition module
  (wires + timing curve) → fuel, in dependency order.

**This graph *is* the engine's data model.** An editor's in-memory document should mirror it
directly rather than invent a parallel schema, because the target output is exactly this graph
serialized as `.mr` text.

**Important limitation found during research:** there is no existing introspection API for
"what inputs does this node type have, with what type/default." `es_script::Node`
(`scripting/include/node.h:72`) keeps its input map private with no accessor,
and piranha's base `Node::getInput()` (`dependencies/submodules/piranha/include/node.h:108-109`)
exposes only a `name` — no type, no default, no range. **Field metadata for the editor has to be
authored as new code**, not scraped from the existing `registerInputs()` calls. See §4.2.

### 2.2 Runtime object model

Compiled nodes produce plain C++ objects deriving from `Part` (`include/part.h`) — `Engine`,
`Crankshaft`, `CylinderBank`, `CylinderHead`, `Piston`, `ConnectingRod`, `Intake`,
`ExhaustSystem`, `Turbocharger`, `IgnitionModule`, `Fuel`, plus `Vehicle`/`Transmission` outside
the engine proper. These are the objects `PistonEngineSimulator` (`include/piston_engine_simulator.h`)
actually simulates. Turbocharger's `Parameters` (`include/turbocharger.h:32-93`, extended this
session with `turbineInducerDiameter`/`turbineExducerDiameter`) is a representative example of
the shape every node's backing struct has: physical quantities with sane defaults, comments
explaining derived behavior.

### 2.3 Visual layer

`SimulationObject` (`include/simulation_object.h:10`) is the render/geometry base class —
`generateGeometry()` / `render()` / `process()` — completely separate from `UiElement`.
Concrete subclasses (`PistonObject`, `CrankshaftObject`, `CylinderBankObject`,
`CylinderHeadObject`, `ConnectingRodObject`, `CombustionChamberObject`) each hold a raw pointer
to their backing `Part` (e.g. `PistonObject::m_piston`).

`EngineSimApplication::createObjects(Engine*)` (`src/engine_sim_application.cpp:817-855`) is the
factory: for every cylinder/crankshaft/bank it `new`s the right object, calls `initialize(this)`,
and assigns the backing pointer. `destroyObjects()` (`:857-864`) is the teardown. **This factory
is exactly what a live editor preview needs and can be reused as-is** — point it at a
scratch `Engine` built from the in-progress document and it renders the same way the main view
does.

### 2.4 UI toolkit — the actual gap

`UiElement` (`include/ui_element.h:11`) / `UiManager` (`include/ui_manager.h:9`) is a small
retained-mode tree: manual `setLocalPosition()` per child, immediate-mode draw primitives
(`drawBox`, `drawFrame`, `drawText`), mouse click/drag/scroll callbacks, all rendered through the
`delta-studio` submodule's `dbasic` layer. There is **no layout engine** (every existing cluster
hand-computes pixel offsets) and — confirmed by an exhaustive grep — **no slider, text-entry,
dropdown, or numeric-input widget anywhere in the codebase**. `UiButton` (`include/ui_button.h`)
and `Gauge` (`include/gauge.h`) are the only interactive/display primitives that exist.
Building the editor means building this widget layer first; it's the single biggest piece of net
new code this project requires (see §5).

### 2.5 Reload cost and the live-mutation precedent

`EngineSimApplication::loadEngine()` (`src/engine_sim_application.cpp:688-764`) is a full
teardown/rebuild: `destroyObjects()`, release and delete the `Simulator`, delete
`Vehicle`/`Transmission`/`Engine`, then rebuild every rigid body, constraint, and geometry object
from scratch, plus re-read every exhaust system's impulse-response `.wav` from disk
(`:744-761`). This is *not* incremental and is too slow to call on every keystroke/slider-drag.

However, the app already has a **live-mutation precedent** for a handful of parameters that
don't require rebuilding the physics system: ignition on/off
(`getIgnitionModule()->m_enabled = ...`, `:1210`), gear changes
(`getTransmission()->changeGear(...)`, `:1220`), clutch pressure ramped every frame
(`:1237-1241`), audio volume via `synthesizer().getAudioParameters()` mutate-and-set
(`:1023-1026`). This establishes the pattern an editor's "instant apply" path should follow for
simple numeric knobs — but only fields that don't change rigid-body topology (bore/stroke/rod
length/journal angles cannot be hot-patched this way; they're baked into constraint geometry at
`loadSimulation()` time). The MVP editor should not attempt this distinction — see §6.

### 2.6 File dialogs

`loadScript()`/`loadTheme()` use raw Win32 `GetOpenFileNameA` (`commdlg.h`, included at
`engine_sim_application.cpp:26-27`) with no cross-platform abstraction. Themes
(`assets/themes/*.mr`) are piranha scripts too, compiled with the same `es_script::Compiler` and
applied via `ApplicationSettings` — confirming that "compile a `.mr` file at runtime and apply
the result" is already a first-class, well-worn code path, not something novel the editor would
be introducing.

## 3. Core design decision: `.mr` text is the save format

Do **not** invent a new binary/JSON engine format. Reasons:

- Every existing asset (`assets/engines/**/*.mr`) stays directly openable in the editor with no
  migration step.
- The compiler, error reporting, and `main.mr`/theme-loading conventions all already assume
  `.mr`. A second format means either two parallel loaders forever, or an eventual forced
  migration.
- It keeps hand-editing and visual-editing interoperable: a user can tweak a generated file by
  hand afterward, or hand-write most of an engine and finish it in the editor (with the caveat
  in §3.1).

The editor's job is therefore: **(a)** hold an in-memory document that mirrors the node graph
(§4), **(b)** serialize that document to `.mr` text, **(c)** feed that text through the existing
`Compiler` → `loadEngine()` path to preview/run it, exactly like `loadScript()` does today for a
picked file.

### 3.1 Round-trip limitation (accept it, don't hide it)

`.mr` is a real language — imports, private helper nodes, arithmetic on inputs, reused
sub-graphs (see how `k20a_turbo.mr` and `renault_ef1_turbo.mr` each define a private
`*_camshaft_builder` node and reuse it per bank). The editor's document model only covers a
*flat* instantiation of the known node types with literal values. Two consequences to design
around, not paper over:

- **Import** only works reliably on engines that were themselves generated by the editor, or
  that happen to already be flat/literal. Importing an arbitrary hand-written asset should be
  best-effort: read back current values off the *compiled runtime objects* (most already have
  getters; a few need adding, mirroring what was just done for
  `Turbocharger::getTurbineInducerDiameter()`), populate the document, and clearly warn the user
  that saving will replace the file's structure (private helpers, reused sub-graphs, comments)
  with a flat, editor-authored equivalent.
- **Save** always emits the editor's own canonical `.mr` shape. Never attempt to preserve or
  diff against arbitrary source text.

## 4. Data model

### 4.1 `EditableEngineDocument`

A plain, serializable C++ struct tree mirroring the node graph from `EngineNode::buildEngine()`,
one struct per node type, field-for-field matching each `*_node.h`'s `registerInputs()` list
(reuse the existing `Parameters` structs directly where possible — e.g.
`Turbocharger::Parameters` already *is* the right shape, no reason to redefine it):

```
EditableEngineDocument
├─ EngineDoc            (name, redline, starter, fuel, sim params — engine_node.h:174-190)
├─ vector<CrankshaftDoc>
├─ vector<CylinderBankDoc>
│   ├─ CylinderHeadDoc
│   │   ├─ FunctionDoc intakeFlow / exhaustFlow   (flow-vs-lift sample tables)
│   │   └─ ValvetrainDoc → CamshaftDoc (lobes: duration/lift/gamma/center)
│   └─ vector<CylinderDoc>  (piston, rod, rod journal, intake, exhaust, wire)
├─ vector<TurbochargerDoc>   (turbine_flow_rate, turbine_inertia, ..., inducer/exducer diameter)
├─ IgnitionModuleDoc         (timing curve, rev limit, wire→angle connections)
├─ FuelDoc
├─ VehicleDoc
└─ TransmissionDoc           (gear ratios, straight_cut, max_clutch_torque)
```

### 4.2 Field metadata (new infrastructure)

Since reflection off `registerInputs()` isn't available (§2.1), each editable field needs a
small, hand-authored descriptor — this is the piece of net-new plumbing that makes generic
form-building possible instead of hand-coding one UI panel per field:

```cpp
struct FieldMeta {
    const char *scriptName;   // must match the *_node.h addInput() key exactly
    const char *label;
    const char *unitSuffix;   // "mm", "psi", "rpm", "" ...
    double toDisplay;         // internal-unit -> display-unit scale (e.g. 1/units::mm)
    double min, max;          // for sliders/clamping; not enforced as hard limits, just UI range
    const char *tooltip;      // pulled from the *_node.h/Parameters comment where one exists
};
```

A `FieldMeta` table per node type (`TurbochargerFields[]`, `CrankshaftFields[]`, ...) is both the
form generator's input *and* doubles as documentation — the comments already in `turbocharger.h`
are close to tooltip-ready text.

### 4.3 Serializer

One function per node type, `writeXxx(std::ostream&, const XxxDoc&, ...)`, producing the same
shape the hand-written assets use (see `k20a_turbo.mr` as the template: `wires`, camshaft
builder, head, main engine node, turbo wrapper, vehicle, transmission, `main`). This is
string-templating, not a real code-generation framework — keep it that simple.

## 5. New UI widgets needed

All new, built on `UiElement`:

- **NumberField** — click to focus, keyboard entry, unit suffix label, min/max clamp from
  `FieldMeta`. (Keyboard text input doesn't exist in the app today at all — check what
  `ysKey`/`ysEngine` keyboard-text APIs `delta-studio` already exposes for menu/text use before
  building a custom key-to-char mapper.)
- **Slider** — drag with `onDrag()` (already a `UiElement` virtual, §2.4), live value readout,
  used for continuous tuning (bore, boost, gear ratios) paired with a `NumberField` for exact
  entry.
- **Toggle/Checkbox** — booleans (`flip_display`, `straight_cut`, ...).
- **Dropdown/EnumPicker** — for choice fields (impulse response selection, fuel presets).
- **CurveEditor** — draggable-point 2D graph, used for three distinct things that all reduce to
  "table of (x, y) samples": intake/exhaust flow-vs-lift curves, cam lobe profiles, ignition
  timing-vs-rpm curves. Worth building once, generically, rather than three bespoke widgets.
  `OscilloscopeCluster`/`Oscilloscope` (`include/oscilloscope.h`) already draws live sampled
  curves and is a useful reference for the plotting math even though it's read-only.
- **TabPanel / CollapsibleSection** — layout container; with no existing layout engine (§2.4),
  even this "just" grouping widget needs its own simple vertical-stack positioning helper so
  panel authors aren't hand-computing pixel offsets for every field (worth doing before the
  widget count grows — a `VStack`-style helper that lays out children top-to-bottom with a fixed
  row height would remove most of the boilerplate every other cluster currently repeats).

## 6. Editing → simulation feedback loop

**MVP (recommended starting point):** every edit — through however many widgets — mutates the
in-memory `EditableEngineDocument` only. An explicit **Apply** action (button, or debounced
~300ms after the last edit) does: serialize document → `.mr` text → write to a temp file (or, if
piranha supports parsing from an in-memory string — confirm via `IrCompilationUnit::parse()`
overloads seen in `dependencies/submodules/piranha/src/ir_compilation_unit.cpp:48-53` — compile
directly from the string, skipping the temp file) → `Compiler::compile()`/`execute()` →
`loadEngine()`. This reuses 100% of the existing, correct, well-exercised reload path and adds
zero new simulation-correctness risk. The cost is a full rebuild + audio re-init per Apply
(§2.5), which is fine for "release the slider, see the result" but not for real-time drag
feedback.

**Stretch goal (later phase, not MVP):** for the subset of fields that are known-safe to
hot-patch (matching the precedent in §2.5 — ignition, gear, clutch, volume-style leaf values
that don't touch rigid-body geometry), write direct setters and skip the reload for those
specific fields during a drag, falling back to full reload on drag-release for everything else.
Don't attempt to generalize this until the MVP's full-reload path is shipped and its actual
latency is measured — it may simply be fast enough that this optimization isn't worth the
per-field-classification complexity it adds.

## 7. Panel breakdown

- **Engine basics** — name, redline, starter torque/speed, sim frequency/noise/jitter, fuel
  preset.
- **Crankshaft & cylinder banks** — bank angle, bore/stroke/rod/journal layout, with a **live 2D
  preview** built from §2.3's `createObjects()` reuse, so bank-angle/journal-angle edits are
  visually obvious immediately (this is the single highest-value visual-editor feature — text
  fields for crank geometry are notoriously hard to reason about blind).
- **Cylinder head** — chamber volume, runner volume/cross-section, intake/exhaust flow curves
  (`CurveEditor`), cam lobe editor (duration/lift/gamma/center, per intake+exhaust).
- **Intake & exhaust** — plenum volume/cross-section, flow rates, runner length, idle throttle
  position.
- **Turbocharger** — flow rate, inertia, efficiencies, max boost, spool speed, spin-down time,
  **inducer/exducer diameter** (this session's addition — first node to fully exercise the new
  field-metadata pattern, good starting point for implementation since it's small and
  self-contained, see §11 Phase 1).
- **Ignition** — timing-vs-rpm curve (`CurveEditor`), rev limit/limiter duration, wire→cylinder
  firing-angle connections (this one probably needs a bespoke small widget — a circular firing
  wheel diagram rather than a generic curve/table).
- **Vehicle & transmission** — mass/drag/tire/diff, gear ratio list (add/remove/reorder), max
  clutch torque, `straight_cut` toggle.
- **Validation/errors** — see §8.

## 8. Validation & error handling

Today, compile errors go only to `error_log.log` on disk (`Compiler::compile()`,
`scripting/src/compiler.cpp:32-59`) — invisible in the GUI. Runtime evaluation failures are
silently swallowed (`Compiler::execute()`, `:61-69`, literally `// Todo: Runtime error`). Since
the editor's Apply path (§6) will produce and compile fresh text on essentially every edit, it
needs `Compiler::compile()`'s error path surfaced into a UI panel instead of a log file, and the
silent runtime-failure case fixed or at minimum turned into a visible "engine failed to build"
message — right now a broken script just silently keeps whatever was previously loaded
(`loadScript()`, `:913-920`), which would be confusing inside an editor (edit → Apply → nothing
visibly happens).

## 9. Persistence & file management

- Extend the existing `GetOpenFileNameA`/`GetSaveFileNameA` Win32 pattern (§2.6) for
  "Save Engine As..." — no new cross-platform dependency needed since the app is already
  Windows-only at this layer.
- **Undo/redo**: doesn't exist anywhere in the app today. Simplest viable approach given the
  document is a plain struct tree: snapshot-based undo (push a copy of
  `EditableEngineDocument` onto a stack before each Apply/discrete edit) rather than a
  command/diff system — the document is small enough that copying it is cheap, and this avoids
  writing per-field-type undo commands.
- **Autosave**: write the in-progress document to a scratch `.mr` on an interval, so a crash
  (this is a physics/audio C++ app with a live rendering+audio thread — crashes during
  experimentation are a realistic risk) doesn't lose an edit session.

## 10. Testing strategy

- **Round-trip / regression tests**: this session's approach of driving
  `es_script::Compiler` headlessly (compile → `execute()` → inspect the resulting `Engine`/
  `Turbocharger`/etc. object graph directly, no GUI needed) is exactly the right pattern for
  testing the serializer — generate `.mr` text from a known `EditableEngineDocument`, compile
  it, and assert the runtime object's fields match what was in the document. (Note from this
  session: compiling a file that only *defines* `public node main {}` without invoking it
  produces a successful `compile()` but a null `output.engine` from `execute()` — any test
  harness for the editor's generated files needs the same `import "generated.mr"\nmain()\n`
  wrapper step `EngineSimApplication::autoWireEngineScript()` already does for hand-picked files,
  `engine_sim_application.cpp:41-90`.)
- **Widget tests**: light — `UiElement`'s mouse-event virtuals are simple enough to unit-test
  the interaction logic (drag → value delta) without a real window, if kept decoupled from
  rendering.
- **Golden-file tests**: import each shipping asset in `assets/engines/**/*.mr` that's flat
  enough to round-trip (§3.1), re-serialize, re-compile, and diff key runtime values
  (redline, cylinder count, firing order) against the original compile — catches serializer
  regressions cheaply.

## 11. Phased rollout

1. **Read-only inspector.** Import (compiled-object → `EditableEngineDocument`) and display
   only, no editing, no save. Validates the document model and the live-preview reuse (§2.3)
   without touching the save format at all. Lowest-risk phase to ship first.
2. **Single-node editing: Turbocharger.** Small, self-contained, already has the newest/most
   complete field set (including inducer/exducer). Full Apply→reload loop (§6 MVP), no curve
   editors needed yet. Proves the serializer + FieldMeta + NumberField/Slider pattern end to end
   on a real node type.
3. **Crankshaft/cylinder-bank editing + live 2D preview.** The highest-value visual feature;
   also the most structurally complex (adding/removing cylinders changes array sizes across
   several sibling structs, per `EngineNode::buildEngine()`'s cylinder-count bookkeeping).
4. **Curve editors**: flow tables, cam lobes, ignition timing — one generic `CurveEditor`
   widget reused three ways (§5).
5. **Full save/open, undo/redo, autosave** (§9).
6. **Stretch: partial live-apply** for hot-patchable fields (§6), only after measuring whether
   MVP's full-reload latency actually needs it.

## 12. Open risks / questions to resolve before implementation

- **Reload latency while editing.** Needs a real measurement (the existing
  `m_performanceCluster->addTimePerTimestepSample()` infra could report this) before deciding
  whether Phase 6's optimization is necessary at all.
- **Audio thread races during Apply.** `loadEngine()` already stops/restarts the audio render
  thread as part of its teardown (§2.5) — confirm this is sufficient under editor-driven
  rapid-fire Apply calls (e.g. a user mashing a slider) or whether Apply needs its own
  debounce/cancellation-in-flight guard.
- **Import fidelity for hand-written assets** (§3.1) — decide product-wise whether import even
  ships in early phases, given it's inherently lossy for anything using private helper nodes
  (which is most of `assets/engines/atg-video-2/*`, including this session's
  `k20a_turbo.mr`/`renault_ef1_turbo.mr`).
- **Keyboard text entry** — confirm what `delta-studio`/`ysEngine` already provides for
  character input before designing `NumberField`; this may already exist at the engine layer
  even though nothing in `engine-sim`'s own code uses it yet.
