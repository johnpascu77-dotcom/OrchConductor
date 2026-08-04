# OrchConductor Phase 4H - Processor Construction-Time JSON Load Probe

## Status

Phase 4H adds a processor construction-time JSON load probe behind the existing runtime JSON preset feature gate.

This is a probe only.

Phase 4H does not switch the runtime preset source.

Phase 4H does not use JSON-backed data for MIDI output.

Phase 4H does not use JSON-backed data for UI labels.

Phase 4H does not change preset ordering.

Hardcoded factory behavior remains authoritative.

## Modified Files

```text
CMakeLists.txt
Source/OrchConductorProcessor.h
Source/OrchConductorProcessor.cpp
```

## Added Files

```text
Tools/OrchConductorProcessorJsonProbeCheck.cpp
Docs/OrchConductor_Phase4H_Processor_Construction_Time_JSON_Load_Probe.md
```

## Processor Probe

Phase 4H adds a construction-time probe to:

```cpp
OrchConductorAudioProcessor::OrchConductorAudioProcessor()
```

When the feature gate is ON, the constructor calls:

```cpp
orchconductor::RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled()
```

The processor stores only probe diagnostics:

```text
Whether the runtime JSON boundary loaded successfully
Whether hardcoded fallback is required
A diagnostic message
```

The processor does not use the loaded JSON library for runtime behavior.

## Diagnostic Accessors

Phase 4H adds read-only diagnostic accessors:

```cpp
bool wasRuntimeJsonPresetProbeLoaded() const;
bool doesRuntimeJsonPresetProbeRequireFallback() const;
juce::String getRuntimeJsonPresetProbeDiagnostic() const;
```

These accessors are intended for developer verification and future diagnostic visibility.

They do not affect audio processing, MIDI output, UI labels, preset IDs, or state serialization.

## Feature Gate OFF Behavior

When built with:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
```

the processor constructor:

```text
Does not call RuntimePresetSource.
Does not parse embedded JSON.
Reports probe loaded = false.
Reports fallback required = true.
Stores a disabled diagnostic message.
```

## Feature Gate ON Behavior

When built with:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

the processor constructor:

```text
Calls RuntimePresetSource.
Allows RuntimePresetSource to load embedded factory JSON.
Stores success/failure diagnostics.
Does not fail construction on JSON failure.
Keeps hardcoded presets authoritative.
```

## Runtime Safety

Phase 4H does not add JSON work to:

```text
processBlock()
prepareToPlay()
releaseResources()
MIDI send path
UI paint path
UI timer path
```

The probe runs only during processor construction.

No disk JSON loading is added.

The factory JSON source remains embedded binary data.

## Developer Verification Target

Phase 4H adds:

```text
OrchConductorProcessorJsonProbeCheck
```

The check constructs an `OrchConductorAudioProcessor` and verifies:

```text
Probe diagnostic message is present.
OFF build does not load JSON and requires fallback.
ON build loads JSON and does not require fallback.
Hardcoded initial preset IDs remain unchanged.
Hardcoded preset names remain unchanged.
Hardcoded strings Low Strings value behavior remains unchanged.
Hardcoded Solo English Horn Lament combi value behavior remains unchanged.
```

## Expected Successful Output

OFF/default build:

```text
[PASS] Processor construction-time JSON probe OFF verification completed successfully.
[PASS] Hardcoded preset behavior remains authoritative.
```

ON build:

```text
[PASS] Processor construction-time JSON probe ON verification completed successfully.
[PASS] Hardcoded preset behavior remains authoritative.
```

## Runtime Non-Adoption

Phase 4H intentionally does not:

```text
Populate preset names from JSON
Populate preset values from JSON
Populate combi presets from JSON
Change MIDI output
Change UI behavior
Change state serialization
Change preset limits
Change editor dropdown behavior
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4I: Processor JSON Probe Diagnostics Regression Verification
```

Suggested Phase 4I scope:

```text
Add broader developer checks proving processor behavior is identical with the gate OFF and ON.
Exercise MIDI output for selected hardcoded section/combi presets.
Confirm JSON probe diagnostics do not affect processBlock output.
Keep JSON-backed preset adoption disabled.
```

Alternative next phase:

```text
Phase 4I: Runtime JSON Adoption Plan
```

This would document the final switch strategy before enabling JSON-backed data.

## Completion Criteria

Phase 4H is complete when:

```text
Processor construction-time probe exists.
Probe is compiled behind ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS.
OFF/default build reports fallback-required without loading JSON.
ON build loads embedded JSON through RuntimePresetSource.
JSON probe failure remains non-fatal.
Hardcoded preset behavior remains authoritative.
Developer processor probe check passes in default/OFF build.
Developer processor probe check passes in explicit ON build.
No audio-thread JSON parsing is introduced.
Editor files remain untouched.
The phase is committed and tagged.
```

Recommended tag:

```text
phase-4H-processor-construction-json-load-probe
```
