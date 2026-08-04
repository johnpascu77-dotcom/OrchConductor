# OrchConductor Phase 4G - Runtime JSON Boundary Parity Verification

## Status

Phase 4G adds developer-only parity verification for the runtime JSON preset source boundary introduced in Phase 4F.

Phase 4G remains passive.

Phase 4G does not connect JSON-backed presets to the plugin processor or editor.

Phase 4G does not change plugin runtime behavior.

Phase 4G does not change MIDI behavior.

Phase 4G does not change UI behavior.

## Modified Files

```text
CMakeLists.txt
```

## Added Files

```text
Tools/OrchConductorRuntimePresetSourceParityCheck.cpp
Docs/OrchConductor_Phase4G_Runtime_JSON_Boundary_Parity_Verification.md
```

## Verification Target

Phase 4G adds:

```text
OrchConductorRuntimePresetSourceParityCheck
```

The target verifies the behavior of:

```cpp
orchconductor::RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled()
```

## OFF Behavior

When built with:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
```

the parity check verifies:

```text
Runtime boundary does not load JSON.
Runtime boundary requires hardcoded fallback.
Runtime boundary reports a non-empty diagnostic message.
```

Expected final line:

```text
[PASS] Runtime JSON boundary OFF parity verification completed successfully.
```

## ON Behavior

When built with:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

the parity check verifies:

```text
Runtime boundary loads embedded JSON.
Runtime boundary does not require hardcoded fallback after successful load.
Runtime boundary reports a non-empty diagnostic message.
Parsed schema matches orchconductor.library.
Parsed schemaVersion is 1.
Parsed libraryName is OrchConductor Factory Library.
Parsed pluginTarget is OrchConductor.
MIDI channel is 1.
MIDI CC range is 20..54.
Reserved CC49 policy is present.
Woodwinds preset count is 20.
Brass preset count is 17.
Percussion preset count is 9.
Strings preset count is 14.
Combi preset count is 29.
All preset factoryId values match their array indices.
All preset values are inside configured MIDI CC and value ranges.
No preset value targets reserved CC49.
Known all_off entries exist and contain zero values.
strings.low_strings contains CC52 = 64.
solo.english_horn_lament contains CC54 = 64.
Final library isValid() check passes.
```

Expected final line:

```text
[PASS] Runtime JSON boundary ON parity verification completed successfully.
```

## Runtime Non-Adoption

Phase 4G intentionally does not modify:

```text
Source/OrchConductorProcessor.cpp
Source/OrchConductorEditor.cpp
```

Therefore:

```text
Hardcoded factory behavior remains authoritative.
Plugin startup behavior remains unchanged.
Preset source remains unchanged.
Preset ordering remains unchanged.
MIDI output remains unchanged.
UI behavior remains unchanged.
```

## Realtime Safety

Phase 4G adds only a developer verification executable.

No new audio-thread work is introduced.

No JSON parsing is added to the realtime path.

No disk I/O is added to the plugin runtime path.

## Relationship To Previous Checks

Phase 3I verified disk JSON parity.

Phase 4E verified the embedded JSON accessor.

Phase 4F introduced the runtime-facing boundary.

Phase 4G verifies that the runtime-facing boundary can expose the same expected factory library shape when the feature gate is ON, while still reporting safe fallback-required behavior when the feature gate is OFF.

## Recommended Next Phase

Recommended next phase:

```text
Phase 4H: Processor Construction-Time JSON Load Probe Behind Feature Gate
```

Suggested Phase 4H scope:

```text
Call RuntimePresetSource during processor construction only when the feature gate is ON.
Store only safe diagnostics or an internal optional library snapshot.
Do not switch preset source yet.
Do not change UI behavior.
Do not change MIDI output.
Keep hardcoded presets authoritative.
Ensure all failures are non-fatal and fallback to hardcoded behavior.
Document construction-time safety boundaries.
```

A more conservative alternative is:

```text
Phase 4H: Runtime Boundary Integration Plan
```

That alternative would document the processor construction-time integration before implementing it.

## Completion Criteria

Phase 4G is complete when:

```text
RuntimePresetSourceParityCheck target exists.
Default/OFF build verifies fallback-required behavior.
Explicit OFF build verifies fallback-required behavior.
Explicit ON build verifies runtime boundary loaded-library parity.
Existing developer checks still build.
Processor/editor files remain untouched.
Default build succeeds.
Explicit OFF build succeeds.
Explicit ON build succeeds.
The phase is committed and tagged.
```

Recommended tag:

```text
phase-4G-runtime-json-boundary-parity-verification
```
