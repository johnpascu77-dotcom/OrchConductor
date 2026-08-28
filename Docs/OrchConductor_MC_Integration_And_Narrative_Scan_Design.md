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

| CC | Parameter | Notes |
|---|---|---|
| `102` | `narrativePosition` | 0..127 → 0.0..1.0 |
| `103` | `narrativeLane` | 0..127 → nearest lane index (clamp to `laneCount-1`) |
| `104` | `authorityMode` | optional; 0..42 = Manual, 43..85 = Combi, 86..127 = Narrative Scan |

OrchConductor listens for these on its **input MIDI**, on a configurable channel (default: **any**,
since it currently ignores all input). Because they are plugin parameters too, a user who prefers
Bitwig-native routing can ignore the CC path entirely and drive `narrativePosition` with an
automation lane or a Bitwig modulator — no MC required.

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

All 7 acceptance criteria below are met. The one deferred design choice (§10) — live CC vs. exported
automation for the MC bridge — is still open and does not block anything; the parameter contract
stands either way. Bridge CC listening (§5, CC102-104) is **not yet implemented** — only the
automatable parameters exist so far. That, and the MC-side emit (§8), are the remaining work.

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

## 8. MC-side work (later, small)

Once 10F.5 is live:

- MC computes a `narrativePosition` from the blueprint playhead (elapsed / total, or a dedicated
  blueprint curve — MC's call).
- MC emits it as **CC102** via the existing `CCDispatcher`, on the dedicated narrative routing.
- Optionally emit `narrativeLane` (CC103) if the blueprint wants to switch dramaturgical shape
  mid-piece; otherwise the lane is set once in OrchConductor's UI per project.
- No new IPC. No `PatternSyncServer` changes. No two-way state.

This is a MC feature to schedule separately; it does not block OrchConductor 10F.5 and OrchConductor
10F.5 does not depend on it.

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
3. Live-test standalone with a Bitwig modulator / automation lane on `Narrative Position`
   (hand-drag confirmed; sustained modulator run not yet exercised).
4. **NEXT** — decide the bridge transport (§10), then either: add CC102-104 listening to
   OrchConductor, and/or MC-side emit of `narrativePosition` from the blueprint playhead (§8).
5. Full-rig test: MC blueprint → OrchConductor narrative scan → OrchGate instances, notes from a
   mix of MPL and clips.
