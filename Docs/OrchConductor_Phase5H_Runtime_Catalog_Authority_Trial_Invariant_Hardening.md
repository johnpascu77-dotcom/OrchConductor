# OrchConductor Phase 5H Runtime Catalog Authority Trial Invariant Hardening

## Purpose

Phase 5H hardens the diagnostic-only runtime catalog authority trial introduced in Phase 5G.

The goal is to prove that the authority-trial diagnostic remains passive and non-authoritative, while also correcting the selected factory sentinel set used by the trial.

Phase 5H does not adopt the runtime catalog as preset authority.

## Scope

Phase 5H includes:

- correction of the `"[Solo] English Horn Lament"` authority-trial sentinel values,
- MIDI regression assertions for coverage audit state,
- MIDI regression assertions for authority-trial state,
- documentation of the diagnostic invariants.

Phase 5H does not include:

- MIDI routing changes,
- preset authority changes,
- editor/UI changes,
- serialization changes,
- a new CMake feature gate.

## Sentinel Reconciliation

During Phase 5H planning, the authority-trial sentinel for combi preset 28, `"[Solo] English Horn Lament"`, was found to differ from both:

- the Phase 5D payload equivalence sentinel,
- the hardcoded MIDI regression behavior.

The corrected authority-trial values are:

| CC | Expected value |
|----|---------------:|
| 20 | 0 |
| 21 | 0 |
| 22 | 0 |
| 23 | 0 |
| 24 | 0 |
| 25 | 127 |
| 26 | 0 |
| 27 | 0 |
| 28 | 0 |
| 29 | 0 |
| 30 | 0 |
| 31 | 0 |
| 32 | 0 |
| 33 | 0 |
| 34 | 0 |
| 35 | 0 |
| 36 | 0 |
| 37 | 0 |
| 38 | 0 |
| 39 | 0 |
| 40 | 0 |
| 41 | 0 |
| 42 | 0 |
| 43 | 0 |
| 44 | 0 |
| 45 | 0 |
| 46 | 0 |
| 47 | 0 |
| 48 | 0 |
| 49 | 0 |
| 50 | 0 |
| 51 | 0 |
| 52 | 127 |
| 53 | 127 |
| 54 | 64 |

These values align the Phase 5G authority-trial sentinel with the existing payload equivalence sentinel and hardcoded MIDI behavior.

## MIDI Regression Invariants

The MIDI regression check now verifies that runtime diagnostic state is present and consistent for:

- runtime catalog coverage audit,
- runtime catalog authority trial.

The authority-trial diagnostic state is build-dependent:

| Build configuration | Expected authority-trial state |
|--------------------|--------------------------------|
| Runtime JSON OFF | not run, not passed, blocked |
| Runtime JSON ON, fallback catalog | not run, not passed, blocked |
| Runtime JSON ON, source-backed catalog, authority trial OFF | not run, not passed, blocked |
| Runtime JSON ON, source-backed catalog, authority trial ON | run, passed, not blocked |

In all cases, MIDI output remains governed by the existing hardcoded processor behavior.

## Safety Notes

Phase 5H is passive.

No code path uses `didRuntimeCatalogAuthorityTrialPass()` to select preset authority, route MIDI, alter state serialization, or change editor behavior.

The runtime catalog remains diagnostic-only.

## Validation Matrix

Recommended validation matrix:

```text
cmake -S . -B build-phase5h-off
cmake --build build-phase5h-off --config Debug
.\build-phase5h-off\OrchConductorProcessorJsonProbeCheck_artefacts\Debug\OrchConductorProcessorJsonProbeCheck.exe
.\build-phase5h-off\OrchConductorProcessorMidiRegressionCheck_artefacts\Debug\OrchConductorProcessorMidiRegressionCheck.exe
```

```text
cmake -S . -B build-phase5h-json-on -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
cmake --build build-phase5h-json-on --config Debug
.\build-phase5h-json-on\OrchConductorProcessorJsonProbeCheck_artefacts\Debug\OrchConductorProcessorJsonProbeCheck.exe
.\build-phase5h-json-on\OrchConductorProcessorMidiRegressionCheck_artefacts\Debug\OrchConductorProcessorMidiRegressionCheck.exe
```

```text
cmake -S . -B build-phase5h-authority-on -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON -DORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL=ON
cmake --build build-phase5h-authority-on --config Debug
.\build-phase5h-authority-on\OrchConductorProcessorJsonProbeCheck_artefacts\Debug\OrchConductorProcessorJsonProbeCheck.exe
.\build-phase5h-authority-on\OrchConductorProcessorMidiRegressionCheck_artefacts\Debug\OrchConductorProcessorMidiRegressionCheck.exe
```

Expected result:

```text
All probe and MIDI regression tools pass.
```

## Result

PASS on local Windows/MSVC validation.