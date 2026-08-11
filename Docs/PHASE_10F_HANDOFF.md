# OrchConductor Phase 10F Handoff

Date: 2026-08-11

## Current Branch

`phase-10F-narrative-scan-lanes`

## Current Phase

Phase 10F: Narrative Scan lanes and dramaturgical orchestration.

Narrative Scan is being added as a third orchestration authority path, alongside manual section presets and combi preset authority.

## Current Commit Stack

As of this checkpoint, the branch contains these Phase 10F commits:

```text
1402f10 Add narrative lane schema and factory example data
eb82577 Parse narrative lanes into runtime preset catalog
24d198e Verify runtime narrative lane coverage
138819e Add narrative scan resolver
```

Working tree should be clean.

## Build Status

The runtime JSON Debug build passed:

```powershell
cmake --build .\build-runtime-json-on --config Debug
```

The dedicated narrative scan resolver check passed:

```powershell
.\build-runtime-json-on\OrchConductorNarrativeScanResolverCheck_artefacts\Debug\OrchConductorNarrativeScanResolverCheck.exe
```

Expected output:

```text
OrchConductorNarrativeScanResolverCheck passed.
```

## Product Scope

This plugin is intended for private use only, primarily/exclusively inside Bitwig.

It is not intended for public distribution.

Therefore public-release requirements such as installers, broad host validation, commercial documentation, supportability, marketplace polish, and cross-DAW guarantees are out of scope unless explicitly reintroduced later.

Primary target: stable private Bitwig composition tool.

## Current Architecture

The plugin has a developing orchestration authority model:

```text
manual section presets
combi preset authority
narrative scan authority
```

Narrative Scan lanes are curated paths through existing combi presets. A scan value from 0.0 to 1.0 resolves to a combi preset according to lane points.

Hysteresis is used to prevent boundary flicker during automation or slow scan movement.

Default hysteresis margin:

```text
0.03
```

Equivalent to roughly 3% of lane width.

## Phase 10F.1 — Schema and Factory Data

Implemented:

- Added optional top-level `narrativeLanes` to `orchconductor_library.schema.json`.
- Added schema definitions for:
  - `narrativeLane`
  - `narrativeLanePoint`
- Added six initial narrative lanes to `Examples/orchconductor_library_v1.example.json`.

Initial lanes:

```text
organic_build
dark_build
bright_build
heroic_build
suspense
anticlimax
```

Each lane contains ordered points mapping scan positions to combi preset IDs.

## Phase 10F.2 — C++ Model, JSON Parsing, Runtime Catalog Access

Implemented:

- Added `NarrativeLaneDefinition`.
- Added `NarrativeLanePointDefinition`.
- Added optional narrative lane parsing in `PresetLibraryJsonLoader`.
- Added runtime catalog read-only accessors for narrative lane metadata and points.

Important runtime catalog accessors include:

```cpp
getNarrativeLaneCount()
getNarrativeLaneId(int laneIndex)
getNarrativeLaneLabel(int laneIndex)
getNarrativeLanePointCount(int laneIndex)
getNarrativeLanePointPosition(int laneIndex, int pointIndex)
getNarrativeLanePointCombiId(int laneIndex, int pointIndex)
```

A prior namespace issue in `OrchConductorRuntimePresetCatalog.cpp` caused a large MSVC C2888 cascade. It was fixed by ensuring the anonymous namespace closes before member function definitions.

## Phase 10F.3 — Runtime Narrative Lane Verification

Implemented in `Source/OrchConductorProcessor.cpp`.

The existing runtime catalog coverage audit now also verifies narrative lane integrity.

Checks include:

```text
narrative lane count == 6
each lane has non-empty id and label
each lane has at least one point
points are strictly sorted
point positions are within 0.0–1.0
all combi IDs are within valid combi range 0–28
organic_build exists
organic_build has 6 points
anticlimax exists
anticlimax last point resolves to combiId 1
```

Coverage diagnostic text was updated to mention narrative lanes.

No UI, routing, parameter, or MIDI behavior changed in this step.

## Phase 10F.4 — Narrative Scan Resolver

Implemented new files:

```text
Source/OrchConductorNarrativeScanResolver.h
Source/OrchConductorNarrativeScanResolver.cpp
Tools/OrchConductorNarrativeScanResolverCheck.cpp
```

Added CMake integration for:

```text
OrchConductor
OrchConductorNarrativeScanResolverCheck
```

The resolver returns:

```cpp
struct OrchConductorNarrativeScanSelection
{
    int combiId = -1;
    int laneIndex = -1;
    int pointIndex = -1;
    double scanPosition = 0.0;
    bool isValid = false;
};
```

Main API:

```cpp
static OrchConductorNarrativeScanSelection resolve(
    const OrchConductorRuntimePresetCatalog& catalog,
    int laneIndex,
    double scanPosition,
    int previousPointIndex = -1,
    double hysteresisMargin = defaultHysteresisMargin) noexcept;
```

Resolver behavior:

- Clamps scan position to 0.0–1.0.
- Selects nearest lane point when no valid previous point exists.
- Uses midpoint boundaries between neighboring points.
- Applies hysteresis when previous point is valid.
- Returns invalid selection if lane, point, or combi ID is invalid.

Dedicated resolver check verifies:

```text
organic_build at 0.0 -> point 0 -> combi 28
organic_build at 1.0 -> point 5 -> combi 2
anticlimax at 1.0 -> point 5 -> combi 1
bright_build at 0.0 -> point 0 -> combi 14
organic_build hysteresis near point 1/2 boundary stays sticky at 0.28
organic_build hysteresis advances at 0.31
```

## Current State After Checkpoint

Narrative Scan currently exists as:

```text
schema data
factory data
runtime parsed data
runtime validated data
standalone resolver
dedicated verification target
```

Narrative Scan is not yet active in the processor's musical behavior.

It does not yet:

```text
have plugin parameters
appear in the UI
select active authority mode
route resolved combi presets to MIDI
persist lane/scan state in DAW projects
display resolved combi information
```

## Next Recommended Milestone

Next milestone:

```text
Narrative Scan playable private alpha
```

Acceptance criteria:

```text
1. User can select Narrative Scan mode.
2. User can select one of six lanes.
3. User can move scan position 0–100%.
4. Plugin resolves scan to combi presets with hysteresis.
5. Plugin sends combi CC payload only when resolved combi changes.
6. UI clearly displays:
   - active authority mode
   - selected lane label
   - scan value
   - resolved combi ID/name
7. Bitwig save/reopen preserves mode/lane/scan.
```

## Recommended Next Implementation Step

Phase 10F.5 should integrate Narrative Scan into processor state cautiously.

Suggested order:

```text
10F.5A Add passive processor state fields for narrative scan:
       - active narrative lane index
       - scan position
       - last resolved narrative scan point index
       - last resolved combi id

10F.5B Add APVTS/plugin parameters only if needed:
       - authority mode
       - narrative lane
       - narrative scan

10F.5C Resolve narrative scan during processing/update path,
       but do not send duplicate CC payloads if resolved combi did not change.

10F.5D Connect resolved combi to existing combi preset payload sender.

10F.5E Update editor/footer display.
```

Important design decision still open:

```text
Should authority mode, lane, and scan be real automatable plugin parameters,
or internal state controlled by UI only?
```

For Bitwig use, making scan automatable is probably desirable.

Likely parameters:

```text
authorityMode       discrete/manual-combi-narrative
narrativeLane       discrete 0..5
narrativeScan       continuous 0.0..1.0 or 0..100
```

## Important UX Rule

The active authority must be visually obvious.

The user should never wonder:

```text
Why did this preset change?
Why did these CCs send?
Am I in manual mode, combi mode, or narrative scan mode?
```

## Private Bitwig Scope Readiness Estimate

Since this is private and Bitwig-only:

```text
Private Bitwig alpha:        75–85%
Private Bitwig usable tool:  65–75%
Private "done enough":       55–70%
Public release:              out of scope
```

Remaining work is mostly:

```text
processor integration
parameter/state persistence
Bitwig automation behavior
minimal clear UI
musical curation of lanes
```

## Useful Build Commands

Configure runtime JSON build:

```powershell
cmake -S . -B .\build-runtime-json-on -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

Build Debug:

```powershell
cmake --build .\build-runtime-json-on --config Debug
```

Run resolver check:

```powershell
.\build-runtime-json-on\OrchConductorNarrativeScanResolverCheck_artefacts\Debug\OrchConductorNarrativeScanResolverCheck.exe
```

Find resolver check executable if path differs:

```powershell
Get-ChildItem .\build-runtime-json-on -Recurse -Filter "OrchConductorNarrativeScanResolverCheck.exe" |
    Select-Object -ExpandProperty FullName
```

Check branch status:

```powershell
git status
```

View recent commits:

```powershell
git log --oneline -8
```

## Current Clean Checkpoint

At the end of this checkpoint:

```text
branch: phase-10F-narrative-scan-lanes
working tree: clean
build: passed
resolver check: passed
```
