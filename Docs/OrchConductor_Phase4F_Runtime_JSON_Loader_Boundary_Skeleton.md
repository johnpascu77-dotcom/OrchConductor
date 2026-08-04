# OrchConductor Phase 4F - Runtime JSON Loader Boundary Skeleton

## Status

Phase 4F adds a runtime-facing JSON preset source boundary.

The boundary remains passive.

Phase 4F does not connect the boundary to the plugin processor or editor.

Phase 4F does not switch factory preset behavior.

Phase 4F does not change MIDI behavior.

Phase 4F does not change UI behavior.

## Modified Files

```text
CMakeLists.txt
```

## Added Files

```text
Source/OrchConductorRuntimePresetSource.h
Source/OrchConductorRuntimePresetSource.cpp
Tools/OrchConductorRuntimePresetSourceCheck.cpp
Docs/OrchConductor_Phase4F_Runtime_JSON_Loader_Boundary_Skeleton.md
```

## Runtime Boundary

Phase 4F adds:

```cpp
orchconductor::RuntimePresetSource
```

The main API is:

```cpp
static RuntimePresetSourceResult loadEmbeddedFactoryJsonIfEnabled();
```

The result reports:

```text
Loaded/unavailable status
Parsed preset library when loaded
Diagnostic message
Whether hardcoded fallback is required
```

## Feature Gate Behavior

The boundary is controlled by:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
```

When the gate is OFF:

```text
Embedded JSON is not parsed.
The result is unavailable.
The diagnostic reports that runtime JSON presets are disabled.
Hardcoded fallback is required.
```

When the gate is ON:

```text
Embedded JSON is read through getEmbeddedFactoryJson().
The embedded pointer/size is converted to text.
The text is parsed by PresetLibraryJsonLoader::fromJsonText().
The parsed library must pass isValid().
The result is loaded on success.
The result is unavailable on failure.
Hardcoded fallback is required on failure.
```

## Safety Policy

The boundary does not throw exceptions.

The boundary returns safe failure information through:

```text
RuntimePresetSourceResult
```

Failure cases include:

```text
Feature gate disabled
Embedded JSON unavailable
Embedded JSON empty after text conversion
JSON parse failure
Final library isValid() failure
```

All failures result in:

```text
status = unavailable
requiresHardcodedFallback() = true
```

## Developer Verification Target

Phase 4F adds:

```text
OrchConductorRuntimePresetSourceCheck
```

With the feature gate OFF, expected behavior is:

```text
Runtime JSON preset source does not load.
Hardcoded fallback is required.
Diagnostic message is present.
```

With the feature gate ON, expected behavior is:

```text
Runtime JSON preset source loads.
Hardcoded fallback is not required.
Parsed library has expected metadata.
Parsed library has expected factory counts.
Parsed library passes isValid().
```

## Runtime Non-Adoption

Phase 4F intentionally does not modify:

```text
Source/OrchConductorProcessor.cpp
Source/OrchConductorEditor.cpp
```

Therefore:

```text
Plugin construction behavior is unchanged.
Preset source remains hardcoded.
Preset ordering remains unchanged.
MIDI output remains unchanged.
UI behavior remains unchanged.
```

## Realtime Safety

Phase 4F does not add audio-thread work.

The new boundary is not called by the audio path.

Future runtime adoption must preserve:

```text
No JSON parsing on the audio thread
No disk I/O for factory JSON
No locks in the realtime MIDI path
No partial JSON-backed preset state
Safe fallback to hardcoded presets
```

## Expected Successful Output

OFF build:

```text
[PASS] Runtime preset source OFF verification completed successfully.
```

ON build:

```text
[PASS] Runtime preset source ON verification completed successfully.
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4G: Runtime JSON Boundary Parity Verification
```

Suggested Phase 4G scope:

```text
Add stronger developer-only parity verification for RuntimePresetSource output.
Compare runtime boundary loaded embedded JSON against expected hardcoded factory behavior.
Keep processor/editor untouched.
Do not switch runtime preset source yet.
```

## Completion Criteria

Phase 4F is complete when:

```text
RuntimePresetSource boundary exists.
Boundary returns disabled/unavailable when the feature gate is OFF.
Boundary loads embedded JSON when the feature gate is ON.
Boundary validates final library isValid().
Developer check target verifies OFF behavior.
Developer check target verifies ON behavior.
Default build succeeds.
Explicit OFF build succeeds.
Explicit ON build succeeds.
Existing JSON checks still pass.
Processor/editor files remain unchanged.
The phase is committed and tagged.
```

Recommended tag:

```text
phase-4F-runtime-json-loader-boundary-skeleton
```
