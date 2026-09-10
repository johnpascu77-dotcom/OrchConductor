# OrchConductor — Narrative Scan Activation & Composer Mastermind Integration Design

Date: 2026-08-28
Status: Design agreed. Not yet implemented.
Branch context: `phase-10F-narrative-scan-lanes` (narrative lane data + resolver + verification exist;
nothing is wired into musical behaviour yet).

---

## 1. Purpose

Two things are being decided here:

1. **How OrchConductor's orchestration evolves over the span of a piece** (the "form / narrative"
   layer), as a feature that works *without* any other plugin.
2. **How Composer Mastermind (MC) drives that layer** when it is present in the project.

These are deliberately kept as one small seam, not two integrations.

---

## 2. The decision, in one paragraph

OrchConductor gains a **Narrative Scan authority mode** driven by a single automatable
`Narrative Position` parameter (0..1), plus a `Narrative Lane` selector. The existing
`OrchConductorNarrativeScanResolver` turns that position into a combi preset; OrchConductor sends
the combi's CC20–CC54 map exactly as it already does for manual combi selection. This is built
**standalone-first** — OrchConductor has no knowledge of MC. MC integration is then just "MC emits
Narrative Position (and optionally Lane) as a CC / automation". The **note path is unchanged and
stays fully source-agnostic** — see §3.

Rejected: direct MC↔OrchConductor IPC (over-engineered for one-way control of one value; couples
OrchConductor to MC's presence). Rejected: moving the narrative logic into MC (clutters MC, and
OrchConductor loses the feature when MC is absent).

---

## 3. Note path — unchanged, no work required

```
<any MIDI source>  ──►  OrchNoteFilter (future)  ──►  OrchNoteMapper  ──►  OrchGate  ──►  instrument
```

`OrchNoteMapper` and `OrchGate` process whatever MIDI arrives on the track. The MIDI source is a
**per-project, per-track routing choice** and always has been:

- MPL instances (via MPL's MIDI out)
- hand-written MIDI clips
- another generator
- any combination of the above, different per track, in the same project

Nothing in this design touches that. "Which note generator" is never an OrchConductor or MC
concern — it is a Bitwig routing decision made when the template/project is assembled.

---

## 4. Control path — the parameter contract

OrchConductor exposes three new pieces of state. All are plugin parameters (automatable, saved in
the project). The first two are additionally addressable by CC for convenience (see §5).

| Name | Type | Range | Meaning |
|---|---|---|---|
| `authorityMode` | choice | `Manual Sections` / `Combi Preset` / `Narrative Scan` | Which source drives the CC20–54 output. Extends the existing implicit manual-vs-combi split. |
| `narrativeLane` | choice / int | `0 .. (laneCount-1)` | Which curated dramaturgical path through the combi library. Currently 6: `organic_build`, `dark_build`, `bright_build`, `heroic_build`, `suspense`, `anticlimax`. |
| `narrativePosition` | float | `0.0 .. 1.0` | Playhead along the selected lane. `0` = lane start, `1` = lane end. |

### Behaviour

- Only active when `authorityMode == Narrative Scan`. In the other two modes these parameters are
  inert (but still saved).
- On each processing update: `resolve(catalog, narrativeLane, narrativePosition, lastPointIndex,
  hysteresisMargin)` → `combiId`.
- If the resolved `combiId` **differs from the last resolved one**, send that combi's full
  CC20–CC54 payload (reuse the existing combi send path). If unchanged, send nothing — no CC spam
  on smooth automation. This mirrors the existing `sendOnPresetChange` discipline.
- `lastPointIndex` and `lastResolvedCombiId` are runtime state, reset on `prepareToPlay` and after
  `setStateInformation`.
- Hysteresis margin: keep the resolver default (`0.03`). Expose as a parameter only if live use
  shows a need.

### Why one value is enough

MC's blueprint knows the whole shape of the piece. It only needs to tell OrchConductor **where we
are in that shape** — a single scalar. The 35-way instrument fan-out is entirely on the
OrchConductor→OrchGate side and is already built and working. The MC→OrchConductor seam is 1 value
(optionally 2–3), not 35.

---

## 5. CC assignment and collision avoidance

### The collision

- **MPL External Control** (driven by MC's `CCMapping.h`): CC `20–24`, `30–35`, `40–45`, `50–55`,
  `60–64`, on **per-instance MIDI channels**.
- **OrchConductor output**: CC `20–54` on **MIDI channel 1** (hardcoded), CC49 reserved for Harp.

These overlap almost completely. They coexist today only because they live on different channels
and, in the template, different routing paths. Any new bridge CC must avoid **both** sets.

### Bridge CC block

Use the MIDI-spec **undefined** controllers, clear of both plugins:

| CC | Parameter | Mapping | Status |
|---|---|---|---|
| `102` | `narrativePosition` | value / 127 → 0.0..1.0 | **live** |
| `103` | `narrativeLane` | value, clamped to the runtime catalog's lane count | **live** |
| `104` | `authorityMode` | <43 = Manual, <86 = Combi, else Narrative Scan | **live** |

OrchConductor listens for these on its **input MIDI, any channel** (`processBlock` →
`applyNarrativeControlCcInput`); the messages pass through untouched. Because they are also plain
plugin parameters, a user who prefers Bitwig-native routing can ignore the CC path entirely and
drive `Narrative Position` with an automation lane or a Bitwig modulator — no MC required.

### Rule for the template

The MC/MPL note+transform traffic and the MC→OrchConductor narrative traffic must not share a
MIDI channel+port. Simplest: OrchConductor's narrative input arrives on its own dedicated
routing (its own note track / a dedicated channel), the way `OrchGate` instances each get their
own single CC today.

---

## 6. OrchConductor build: Phase 10F.5 (standalone, no MC awareness) — DONE

Built and confirmed live in Bitwig 2026-08-28, on `phase-10F-narrative-scan-lanes`:

- `cd4427f` increment 1 — `AuthorityMode` enum + `Authority Mode` / `Narrative Lane` /
  `Narrative Position` parameters, synced to passive state, state version v2→v3.
- `8f4da10` increment 2 — `processBlock` resolves lane+position → combi id and sends the CC20-54
  payload only on combi change; shared `isNarrativeScanDriving()` / `getEffectiveCombiPresetId()` /
  `isEffectiveCombiModeActive()` so the send path and the MIDI-map/output-table previews agree,
  without touching `combiPresetId` or the `combiPreset` parameter. New regression case.
- `d16f94d` increment 3 — Authority Mode selector + Narrative Scan UI section (lane dropdown,
  position slider, resolved-combi status line), authority↔combi-box coherence, removed the stale
  10E overlap panel, editor made resizable (980×800 default).

All 7 acceptance criteria below are met.

**Bridge CC listening (§5, CC102-104) is live** — `64104bc`. And the **MC side is live** — MC's
`narrative-position` synthetic modulator-target dimension (see §8) emits the blueprint playhead's
0→1 progress as a CC through the existing ModulatorTarget path. Confirmed end-to-end in Bitwig
2026-08-28: MC ModulatorTarget CC102 → OrchConductor Narrative Scan walks the selected lane.

The one deferred design choice (§10) — live CC vs. exported automation — is moot now that both work;
the ModulatorTarget path is the live option and the parameter stays automatable for the baked one.

Original increment plan (for reference):

- **10F.5A — passive state.** Add `activeNarrativeLaneIndex`, `narrativePosition`,
  `lastResolvedNarrativePointIndex`, `lastResolvedNarrativeCombiId` as plain members. No parameters,
  no behaviour yet. Reset in `prepareToPlay` / `setStateInformation`.
- **10F.5B — parameters.** Add `authorityMode`, `narrativeLane`, `narrativePosition` to the APVTS /
  parameter layout. Persist. Wire UI-side setters mirroring the existing
  `setSectionPresetIdFromUI` style.
- **10F.5C — resolve in the update path.** When `authorityMode == Narrative Scan`, call the
  resolver each block/update; store the resolved point + combi id; **do not send** if unchanged.
- **10F.5D — connect to the sender.** On change, route the resolved `combiId` through the existing
  combi CC payload path (`getCombiPresetValueForCc` / runtime catalog lookups). "Send All Off"
  and manual "Send Current Presets" continue to work.
- **10F.5E — UI.** Authority mode selector; lane dropdown (populated from the runtime catalog's
  lane labels); a position slider/readout; and a **clear active-authority indicator** so the user
  never wonders why a preset changed. Show the resolved combi name + lane point.
- **10F.5F — verification.** Extend the existing coverage audit / add a processor-level check that
  narrative-scan sends fire only on combi change and that project save/reload restores
  mode+lane+position.

### Acceptance (from the handoff doc, unchanged)

1. User selects Narrative Scan mode. 2. Selects one of the lanes. 3. Moves position 0–100%.
4. Resolves to combis with hysteresis. 5. Sends combi CC payload only on change. 6. UI shows
active mode / lane / position / resolved combi. 7. Bitwig save+reopen preserves all three.

---

## 7. Build-config consequence

`ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS` currently defaults **OFF**. Narrative lanes are parsed
**only** on the runtime-JSON path (`OrchConductorPresetLibraryJson` → `RuntimePresetCatalog`); the
fallback catalog has no lanes. The embedded factory JSON is
`Examples/orchconductor_library_v1.example.json`, which already contains the 6 lanes.

**Phase 10F.5 requires the runtime-JSON build — and that is already the shipping config.** The
installed Phase 10E plugin reports "Catalog: Runtime JSON active" in its status line, i.e. it is
already built with `ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON`. (The still-installed Phase 9B
build reports "Factory fallback active" — it predates / omits the flag and is the pre-runtime-JSON
safety net; it can be retired once 10F is confirmed.) So §7 is not a blocker: the checkpoint in §11
step 1 is just "build the 10F branch as-is, confirm it still behaves like 10E."

### 10F vs 10E — why the built plugin still says "Phase 10E"

The `phase-10F-narrative-scan-lanes` branch adds 5 commits on top of 10E (schema data, JSON
parsing, runtime-catalog accessors, the resolver, its verification target, the handoff doc). It
changes **no editor UI and no processor musical behaviour** — every build label and status string
still says "Phase 10E". A 10F build is visually identical to 10E. That is why 10F is easy to lose
track of; it may never have been built as a distinct artifact. All 10F.5 work continues on this
branch.

---

## 8. MC-side work — DONE

Implemented in Composer Mastermind (`main`), confirmed live 2026-08-28:

- New **synthetic modulator-target dimension** `ComposerCore::kNarrativePositionDimension`
  (`"narrative-position"`). It is *not* an `ArcSet` dimension — `sendModulatorTargetUpdates`
  special-cases it and calls `ComposerCore::getNarrativePositionAt(currentBar)` instead of sampling
  an authored Arc.
- `getNarrativePositionAt` = `(currentBar − minStartBar) / (maxSectionEnd − minStartBar)`, clamped
  0..1; returns 0 when no blueprint is current. Read under `blueprintMutex`.
- The user adds a **Modulator Target** in the Modulators tab: CC `102`, a dedicated MIDI channel,
  mode `arc`, dimension `narrative-position`. It then rides the entire existing ModulatorTarget
  path — per-bar dispatch from `processBar`, `CCMapping::encodeFloat` 0..127, project persistence,
  the Modulators-tab dropdown (offered for targets, not for `ModulationRoute`s — it is not an MPL
  parameter).
- No new IPC. No `PatternSyncServer` changes. No two-way state.
- Lane (CC103) is still set in OrchConductor's own UI per project — MC has no concept of which
  OrchConductor lane a blueprint maps to, so it only emits position.

Dispatch granularity is one update per bar (same as every other arc-driven ModulatorTarget); fine
for section-length narrative movement.

---

## 9. Deferred: Level 2 — metadata matching

A later option: MC sends the raw arc dimensions and OrchConductor picks the nearest-matching combi
by its `NarrativeMetadata` instead of scanning a pre-authored lane.

Not now, because the axes don't line up cleanly:

- MC arc dimensions: energy, tension, density, **complexity, coherence**
- OrchConductor metadata: energy, tension, density, **brightness, weight**

Shared: energy, tension, density. The rest would need a defined mapping and "nearest match" can
jump between distant combis. 1-D lane position (MC owns how it's computed) is the better first
contract. Revisit only if lane-scanning proves too coarse in real use.

---

## 10. Open decisions (not blocking 10F.5A–C)

- **Bridge transport**: live CC102 during playback vs. MC-exported automation clip on the
  `narrativePosition` parameter. Recommendation: support the parameter as the contract; add CC102
  listening as convenience; decide live-vs-baked per project. Low cost, reversible.
- **Lane switching mid-piece**: allow `narrativeLane` automation, or lock it per project? Default
  to allowing it; resolver already clamps.
- **Update rate**: resolve every `processBlock`, or throttle to e.g. every N ms / on transport
  grid? Start with every block (resolver is cheap, send is change-gated); revisit if CC timing
  looks bursty.

---

## 11. Sequencing

1. ~~Checkpoint build of `phase-10F-narrative-scan-lanes`, confirm no regression vs 10E.~~ **DONE**
2. ~~OrchConductor Phase 10F.5 (params+state / resolve+send / UI).~~ **DONE** — `cd4427f`, `8f4da10`,
   `d16f94d`; confirmed live in Bitwig 2026-08-28.
3. ~~Live-test standalone with a Bitwig modulator / automation lane.~~ Hand-drag confirmed.
4. ~~Bridge: CC102-104 listening in OrchConductor + MC-side `narrative-position` emit.~~ **DONE** —
   OrchConductor `64104bc`; MC `narrative-position` modulator dimension. Confirmed: MC ModulatorTarget
   CC102 → OrchConductor Narrative Scan walks the lane.
5. ~~Full-rig test in a populated template.~~ Confirmed with 2 instruments; whole-section rollout
   pending. Surfaced 3 Bitwig-routing lessons + one real CC-collision fix (§12).
6. ~~Harmonic field on the same lane.~~ **DONE** (§13).
7. **NEXT** — whole-section rollout of the Note FX Layer + OrchNoteFilter chain; MC emits its own
   MotifEngine pitch classes as the field (MC-side, not scheduled).

---

## 12. Rig-integration lessons (Bitwig-side)

1. **Stale track taps** — changing OrchConductor's own MIDI input silently invalidates every
   downstream track tapping OC's output (dropdown still shows the source, routes nothing). Fix:
   re-pick the Note Input. Motivates a section MIDI-bus topology (3 bus tracks tap OC, instrument
   tracks tap their bus).
2. **Note Receiver needs an empty second Note-FX layer** — a lone Note Receiver *replaces* the
   track input; add an empty `Layer 2` and MPL notes + OC's CC merge. This is the fan-out
   mechanism — one MPL instance, per-track Note FX Layer, no `OrchNoteDistributor` needed.
3. **CC collision (fixed)** — OC's CC20-54 output overlaps MPL's CC20-64 control map (MPL Rate on
   CC23 was opening the Oboe 1 gate). OC was forwarding its whole input stream. Fix: the
   **`Input Passthrough`** parameter — `Off` / `Control CCs (>= 105)` / `All`, default
   **Control CCs**. It reads the bridge CCs (102-104) as always, then keeps only input controller
   events with `CC >= 105` (MC's field-mask CC110-121 and future high control CCs) and drops the
   rest — so OC blocks the MPL collision zone (20-64) but relays the control plane, letting a
   clip-fed wash track receive both OC's CC20-54 *and* MC's field mask on the single OC wire with
   no Note Receiver. (`68fa9f3` first added this as a 2-state toggle; widened to the 3-way here.)

## 13. Harmonic field on the same lane — DONE (`<this branch>`, 2026-08-29)

- `NarrativeLanePointDefinition` gains optional `pitchFieldIndex` (int; -1 = leave the field
  alone; >= 0 indexes OrchNoteFilter's **append-only** field-preset list, 0 = Chromatic). Schema +
  parser + runtime-catalog accessor. Example `organic_build` walks pentatonic → whole-tone →
  octatonic → Dorian → major → chromatic.
- OrchConductor gains a **`Field Select CC`** parameter (default **105**, 0 = off). When the
  resolved lane *point* changes and its `pitchFieldIndex` differs from the last one sent, OC emits
  `CC105 = round(index / 14 * 127)` next `processBlock`, on channel 1, alongside the CC20-54
  payload, suppressed when unchanged. OrchNoteFilter decodes it back to a preset index.
- One `Narrative Position` automation lane now evolves **orchestration + density + harmonic
  field** together.
- **Coupling**: OC's `maxPitchFieldIndex = 14` ↔ OrchNoteFilter's 15-entry list. Both append-only.

## 14. Narrative lane library — import from JSON (no rebuild) — DONE (`<this branch>`, 2026-09-10)

The 6 narrative-scan lanes live only in the runtime JSON catalog
(`Examples/orchconductor_library_v1.example.json`, embedded). Growing that library used to mean a
rebuild. Now it does not.

- `ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS` is **ON by default** (was OFF), so the embedded
  library's lanes are always available.
- The editor gains **`Export Lane Library JSON`** (writes the built-in factory library out as an
  editable starting template) and **`Import Lane Library JSON`** (loads an edited file, adopts its
  lanes live, and copies it to `<userAppData>/OrchConductor/NarrativeLibrary.json` so it
  auto-loads next launch). Same pattern as the user-combi Export/Import, and the same
  auto-library-file idea as `loadUserCombiPresetsFromUserLibrary()`.
- **Only the narrative lanes + labels of a loaded file take effect.** CC values stay
  hardcoded-authoritative — `runRuntimeCatalogProbe()` adopts the catalog for lanes whenever it is
  a ready, factory-shaped source, but `runtimePresetCatalogAuthorityActive` (values) is still
  gated on `ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL` (OFF). So an imported file with
  edited combi values loads its lanes but does not change MIDI output.
- Rejected imports (missing sections, not a complete `orchconductor_library_v1` document) keep the
  previously-loaded library and say so in the status line.
- **Thread safety**: `runtimePresetCatalog` is now guarded by a `juce::SpinLock` - the audio
  thread reads it in `updateNarrativeScanResolution()` / `tryGetRuntime*()` while the message
  thread can swap it whole on import.
- The narrative lane parameter already supported 16 lanes; no schema change was needed to grow
  past 6.

## 15. Harp / Piano on a lane point — DONE (`<this branch>`, 2026-09-10)

No factory combi (nor the embedded JSON) touches CC49 (Harp) or CC55 (Piano) - they only carry a
value from a *user combi's* `harpValue` / `pianoValue` override. So a Narrative Scan run could
never bring the Harp or Piano in.

- `NarrativeLanePointDefinition` gains optional `harpValue` / `pianoValue` (int, -1 = leave at the
  combi's own value, which is 0 for every factory combi; 0-127 overrides it). Schema + parser +
  runtime-catalog accessors (`getNarrativeLanePointHarpValue` / `...PianoValue`).
- While Narrative Scan drives, the resolved point's `harpValue` / `pianoValue` (when >= 0) replace
  the harp/piano value in the CC payload send, on top of the resolved combi. Stored as
  `lastResolvedNarrativeHarpValue` / `...Piano` (runtime state, reset in `prepareToPlay` /
  `setAuthorityMode`); a change in either forces a resend the same way a combi-id change does.
- The embedded `organic_build` lane now demos it: harp swells 50 -> 80 -> 110 -> 127 across its
  second half, piano enters at 127 on the final tutti point - the same arc shape as the user's
  "Accum" combi sequence, now reachable from a single `Narrative Position` automation lane.

## 16. Harp / Piano as a Manual Sections control - DONE (`<this branch>`, 2026-09-10)

Harp/Piano still have no section-preset table (they aren't part of any of the 4 families' CC
lists), but they now have a live manual control so you can audition a harp or piano solo (or
harp/piano against a manual texture) without going through a combi.

- `manualHarpValue` / `manualPianoValue` (-1 = Off, 0-127), driven by the editor's **Harp** /
  **Piano** sliders (their own row under the section-preset dropdowns). Persisted in plugin state
  (v5 -> v6, backward compatible). `setManual{Harp,Piano}Value` requests a send when
  Send-on-Preset-Change is on, same as a section preset change.
- Precedence in the CC49/CC55 send: **manual sliders** (Manual Sections mode only) >
  **narrative lane point** (Narrative Scan) > **user combi** `harpValue`/`pianoValue` (Combi). In
  Combi / Narrative Scan mode the manual sliders are inert for output (like the section dropdowns).
- **"Save Current Sections as Combi"** now captures the manual Harp/Piano values into the new user
  combi's `harpValue`/`pianoValue` - so "dial a texture with the 4 section dropdowns + Harp/Piano,
  hear it, save it" produces a combi that reproduces exactly that.
- Two instruments from the same family with no named section preset (e.g. "Flute 1 + Oboe 2") are
  reachable via the Combi Grid - see §17.

## 17. Instrument Combi Grid - DONE (`<this branch>`, 2026-09-10)

A second view in the editor ("Conductor" / "Combi Grid" buttons at the top): every one of the 43
instruments OrchConductor emits as a labelled 0-127 slider in one scrollable grid, saved as a user
combi's `explicitCcValues` (which already override every section preset and the harp/piano fields).

- Processor: `getInstrumentSlotCount()` (43) + `getInstrumentSlot{Name,Cc,SectionName,CurrentValue}(i)`
  in send order (woodwinds, brass, percussion, strings, Harp, Piano); `getCombiResolvedCcValue`
  (per-CC resolve for loading a combi into the grid); `getUserCombiExplicitCcValues`;
  `saveInstrumentGridAsUserCombi (name, values, existingUserCombiId = -1)` - new combi or update in
  place (keeps name/section-ids/metadata).
- The grid stores all 43 values explicitly, so a grid combi's output is exactly what the sliders
  show - section composition never leaks in.
- Grid actions: **Save as New Combi**, **Update Selected Combi**, **Load / seed** (any factory or
  user combi - factory seeds read-only, user combis load for editing), **Seed from Current
  Output** (snapshot whatever OrchConductor is emitting right now), **Clear**.
- `InstrumentGridComponent` is a full-area opaque child of the editor, shown/hidden by the view
  switch; no `TabbedComponent` refactor of the existing layout.
- Combi Grid also has a **Randomize** button - `generateRandomGridCombi(style, seed)`, an
  orchestration-aware starting point (Balanced / Feature a section / Sparse / Tutti): strings
  backbone, WW pairs / horn units / low-brass move together, one register favoured + opposite
  wiped, 40-127 value spread, never silent.

## 18. Narrative Lane Maker - DONE (`<this branch>`, 2026-09-10)

A third editor view: build a narrative lane as an ordered list of combi "stops" along the 0..1
`Narrative Position` timeline, and save it straight into the lane library (§14).

- Row = `# | position | combi dropdown | up / down / remove`. "Stops" 2-16 (even auto-spread,
  positions editable), Load-lane dropdown, Save to Lane Library, Delete Lane.
- **Propose** an arc: `generateNarrativeLane(shape, pointCount, restlessness, seed)`. Ten arc
  shapes, each with its own energy + tension curve sampled at every stop; the combi whose
  approximate character (a hardcoded factoryId -> {energy,tension,brightness} table; user combis
  estimated from their CC payload) best matches is chosen from a weighted shortlist.
  - Shapes: Organic Build, Arch (Rise & Fall), Long Fade / Dissolution, Terraced Blocks, Surging
    Waves, Heroic Journey (statement -> dark struggle -> triumph), Suspense -> Release, Mosaic /
    Episodic, Catastrophe / Collapse, Pastoral Plateau.
  - **Restlessness** 0..1: low = the generator holds the previous combi or reprises an earlier
    stop more often ("coherence / intention emulation"); high = always moves on.
  - Also sprinkles a descending harmonic-field walk and harp/piano at the peak stops.
- Processor: `getNarrativeLanePoint{Count,Position,CombiId,FieldIndex,HarpValueAt,PianoValueAt}`,
  `saveNarrativeLane` / `deleteNarrativeLane` (edit the user's NarrativeLibrary.json - or the
  built-in template if none - and re-import live; points sorted + de-collided),
  `getNarrativeArcShapeNames`, `generateNarrativeLane`.

## 19. OrchGate Response Bridge - DONE (`<this branch>`, 2026-09-11)

A broadcast every OrchGate in the rig can follow to randomise its own response - so the user
never ticks/unticks CC Invert (or nudges CC Threshold / participation) on every instance by
hand. Two undefined controllers on channel 1, clear of the CC20-62 send map, CC102-105, and
MPL's CC20-64:

- **CC 106 - Gate Response Mode**: the "chapter" seed. Steps only when the articulation attitude
  should change.
- **CC 107 - Gate Response Amount** (0..127): how far each OrchGate's per-instance randomiser
  may push. 0 = identity / no overlay (also the "Send All Off" state).

Both CC numbers are relocatable params (`gateResponseModeCc` / `gateResponseAmountCc`, 0 = off).

### OrchGate side (`OrchGate` repo)

New APVTS params, all inert unless **Follow Conductor Response** is on (default off - a
standalone OrchGate with host LFOs is unaffected):

- `followConductorResponse`, `responseAffectsInvert` / `responseAffectsThreshold` /
  `responseAffectsParticipation` (per-target opt-in, all default on), `responseModeCc` /
  `responseAmountCc` (default 106 / 107).
- `resolveResponseOverlay()`: `seed = hash(mode value, this instance's gate CC number)`. From
  the seed + amount: Invert flips with probability up to 50% at full amount; Threshold jitters
  +/-24 around the user's value; participation Floor/Ceiling nudge +/-20% each. Every OrchGate
  diverges (its own CC number is in the seed) but they all shift together when CC106 steps, and
  identically after a reload - deterministic, same fail-safe spirit as the closed-by-default
  gate. The manual `ccInvert` / `ccThreshold` stay the base; the overlay rides on top.
- A response CC arriving mid-block re-resolves and re-applies (a flipped invert can change the
  gate open/closed with no gate CC of its own).
- Editor: "Follow Conductor Response" + the three target toggles + Mode/Amount CC sliders + a
  live overlay readout.

### OrchConductor side

Two brains, layered (the panel overrides the lane point while enabled):

- **Narrative lane point**: `NarrativeLanePointDefinition` gains `gateResponseMode` /
  `gateResponseAmount` (-1 = leave alone), read from JSON, round-tripped by `saveNarrativeLane`,
  emitted when the resolved point changes. `generateNarrativeLane` fills them from the arc - the
  amount tracks tension, the mode only advances on a genuine combi move (a held / reprised combi
  keeps its attitude).
- **Live "Gate Response" panel** (Conductor tab): Broadcast toggle, Amount slider, Mode slider,
  **Shuffle** (rolls a new 1..127 mode + arms). `setGateResponseManual{Enabled,Mode,Amount}`,
  `shuffleGateResponseMode`, `getEffectiveGateResponse{Mode,Amount}`. State v7.
- Emission mirrors the field-select CC: on change, re-broadcast on an explicit "Send Current
  Presets", amount 0 on "Send All Off" or when the driving source goes away (back to each
  OrchGate's own knobs). `expectedSendCcCount` unchanged - 106/107 are control-plane like
  102-105, not combi payload.
