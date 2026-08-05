# OrchConductor Phase 5A - Controlled Runtime JSON Authority Trial

## Status

Phase 5A introduces the first controlled runtime JSON authority trial.

The trial is intentionally limited to the developer catalog check.

The plugin runtime remains unchanged.

## Modified Files

```text
Tools/OrchConductorRuntimePresetCatalogCheck.cpp
```

## Added Files

```text
Docs/OrchConductor_Phase5A_Controlled_Runtime_JSON_Authority_Trial.md
```

## Purpose

Phase 5A proves that the runtime JSON catalog can be treated as an authority candidate under controlled conditions.

The trial is active only when:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

The trial is inactive when:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
```

## Authority Boundary

Phase 5A does not make runtime JSON authoritative in the plugin.

The authority trial is limited to:

```text
Tools/OrchConductorRuntimePresetCatalogCheck.cpp
```

The runtime catalog is treated as an authority candidate only inside the developer check.

## OFF Behavior

With runtime JSON disabled, the check verifies:

```text
Runtime source is not loaded.
Runtime source requires hardcoded fallback.
Runtime-source catalog remains fallback-only.
Controlled authority trial is inactive.
```

## ON Behavior

With runtime JSON enabled, the check verifies:

```text
Runtime source is loaded.
Runtime source does not require hardcoded fallback.
Runtime-source catalog is ready.
Runtime-source catalog does not require fallback.
Runtime-source catalog has expected factory shape.
Runtime-source catalog label payloads mirror source data.
Runtime-source catalog value payload counts mirror source data.
Runtime-source catalog typed CC/value payloads mirror source data.
Controlled authority trial payload parity is satisfied.
```

## Non-Adoption Guarantee

Phase 5A does not modify:

```text
CMakeLists.txt
Source/OrchConductorRuntimePresetCatalog.h
Source/OrchConductorRuntimePresetCatalog.cpp
Source/OrchConductorRuntimePresetSource.h
Source/OrchConductorRuntimePresetSource.cpp
Source/OrchConductorProcessor.cpp
Source/OrchConductorProcessor.h
Source/OrchConductorEditor.cpp
Source/OrchConductorEditor.h
Examples/
Schemas/
Scripts/
```

Phase 5A does not change:

```text
Editor combo-box population
Editor displayed preset selection behavior
Processor preset application
MIDI CC generation
MIDI channel behavior
Reserved CC handling
State serialization
Hardcoded factory preset behavior
```

## Safety Position

Phase 5A preserves the existing safety model:

```text
No disk I/O.
No JSON parsing on the audio thread.
Embedded JSON only.
Feature-gated runtime JSON.
Fallback-first behavior.
Hardcoded plugin behavior remains authoritative.
```

## Relationship to Phase 4P

Phase 4P closed passive non-authoritative catalog parity verification.

Phase 5A starts controlled authority proving, but only in a developer check.

This creates a safe bridge before wiring any plugin runtime path to JSON authority.

## Completion Criteria

Phase 5A is complete when:

```text
Default/OFF catalog check builds and passes.
Explicit OFF catalog check builds and passes.
Explicit ON catalog check builds and passes.
Plugin target still builds.
Existing developer checks still build.
Forbidden files remain untouched.
Working tree is clean after commit.
Phase is tagged.
```

## Recommended Commit

```text
Add controlled runtime JSON authority trial check
```

## Recommended Tag

```text
phase-5A-controlled-runtime-json-authority-trial
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 5B: Processor-Side Runtime Catalog Authority Probe
```

Suggested Phase 5B principle:

```text
OFF remains exact legacy hardcoded behavior.
ON may allow one narrow processor-side authority probe.
The probe must not change emitted MIDI yet unless explicitly verified by regression tests.
```
