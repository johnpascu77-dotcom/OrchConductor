# OrchConductor Phase 5B - Processor-Side Runtime Catalog Authority Probe

## Status

Phase 5B introduces a processor-side runtime catalog authority probe.

The probe is passive. The plugin still uses legacy hardcoded behavior for actual preset application and MIDI output.

## Modified Files

```text
Source/OrchConductorProcessor.h
Source/OrchConductorProcessor.cpp
Tools/OrchConductorProcessorJsonProbeCheck.cpp
Tools/OrchConductorProcessorMidiRegressionCheck.cpp
CMakeLists.txt
```

## Purpose

The processor records diagnostic state for a runtime preset catalog authority candidate behind:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
```

## OFF Behavior

```text
Runtime source probe is not loaded.
Runtime source probe requires fallback.
Runtime catalog authority probe is inactive.
Runtime catalog authority probe requires fallback.
Runtime catalog authority probe does not claim expected factory shape.
Hardcoded behavior remains authoritative.
```

## ON Behavior

```text
Runtime source probe reports diagnostic state.
Runtime source probe may load successfully or fall back safely.
Runtime catalog authority probe is ready.
Runtime catalog authority probe has expected factory shape.
Runtime catalog authority probe may use source data or fallback metadata.
Hardcoded behavior remains authoritative.
```

## Fallback-First ON Semantics

Phase 5B is fallback-first.

When the runtime JSON feature gate is ON, the processor-side probe must not fail merely because the embedded runtime source is unavailable or malformed.

Instead, it must verify that:

```text
A diagnostic is reported.
The authority-candidate catalog remains ready.
The authority-candidate catalog reports expected factory shape.
Fallback requirement is observable.
Hardcoded processor behavior remains authoritative.
MIDI output remains unchanged.
```

Full JSON-source payload authority remains the responsibility of catalog-level parity checks and later processor payload-equivalence phases.

## Passive Boundary

Phase 5B does not allow runtime JSON catalog data to drive:

```text
Preset application
MIDI CC generation
Editor combo-box population
Editor selected preset labels
State serialization
Plugin parameter behavior
```

## MIDI Safety

The MIDI regression check verifies the authority probe does not alter the expected 35-CC MIDI emission shape.

## Completion

Phase 5B is complete after OFF/default and explicit ON checks pass, plugin builds, the working tree is clean after commit, and the phase is tagged.

