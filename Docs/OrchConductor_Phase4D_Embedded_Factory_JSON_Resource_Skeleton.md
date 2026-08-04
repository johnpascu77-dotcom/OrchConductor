# OrchConductor Phase 4D - Embedded Factory JSON Resource Skeleton

## Status

Phase 4D adds a passive embedded factory JSON resource skeleton.

The factory JSON file is added to JUCE binary data generation.

A small passive C++ accessor exposes the embedded JSON as a pointer/size pair.

Phase 4D does not parse JSON in the plugin runtime.

Phase 4D does not change preset behavior.

Phase 4D does not change MIDI behavior.

Phase 4D does not change UI behavior.

## Modified Files

```text
CMakeLists.txt
```

## Added Files

```text
Source/OrchConductorEmbeddedFactoryJson.h
Source/OrchConductorEmbeddedFactoryJson.cpp
Docs/OrchConductor_Phase4D_Embedded_Factory_JSON_Resource_Skeleton.md
```

## Embedded Factory JSON Source

The embedded source file is:

```text
Examples/orchconductor_library_v1.example.json
```

This file remains the Phase 3 factory JSON library source.

## Binary Data Target

Phase 4D adds the JUCE binary data target:

```text
OrchConductorFactoryJsonData
```

The target embeds:

```text
Examples/orchconductor_library_v1.example.json
```

The plugin target links this binary data target so the generated binary symbols are available to the plugin build.

## Generated Resource Symbol

The expected generated JUCE BinaryData symbols are:

```cpp
BinaryData::orchconductor_library_v1_example_json
BinaryData::orchconductor_library_v1_example_jsonSize
```

These names are derived from the source filename:

```text
orchconductor_library_v1.example.json
```

Phase 4D intentionally hides direct use of these generated symbols behind a small accessor wrapper.

## Passive Accessor

Phase 4D adds:

```text
Source/OrchConductorEmbeddedFactoryJson.h
Source/OrchConductorEmbeddedFactoryJson.cpp
```

The accessor API is:

```cpp
namespace orchconductor
{
    struct EmbeddedFactoryJson
    {
        const char* data = nullptr;
        int size = 0;

        bool isValid() const noexcept;
    };

    EmbeddedFactoryJson getEmbeddedFactoryJson() noexcept;
}
```

This accessor only exposes embedded data.

It does not:

```text
Parse JSON
Validate JSON
Own preset state
Switch preset sources
Emit MIDI
Modify UI
Modify processor behavior
Modify editor behavior
```

## Compile Gate Relationship

Phase 4B introduced:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
```

Phase 4D does not activate runtime JSON behavior, regardless of whether the gate is ON or OFF.

Current behavior remains:

```text
Gate OFF:
  Embedded resource may exist in the build.
  Runtime does not use it.

Gate ON:
  Compile definition exists.
  Embedded resource may exist in the build.
  Runtime still does not use it yet.
```

A later phase may use the gate to control parsing and runtime adoption.

## Runtime Boundary

The embedded resource accessor is passive.

The runtime boundary remains closed.

Phase 4D does not modify:

```text
Source/OrchConductorProcessor.cpp
Source/OrchConductorEditor.cpp
```

Therefore:

```text
No plugin startup JSON parsing is added.
No preset switching logic is changed.
No hardcoded factory preset behavior is replaced.
No fallback path is needed yet because the resource is not consumed at runtime.
```

## Validation Expectations

The existing Phase 3 validation remains authoritative for the JSON content:

```text
Scripts/Validate-OrchConductorLibrary.ps1
OrchConductorPresetLibraryCheck
```

Phase 4D should still pass:

```text
JSON schema validation
Passive JSON loader verification
Factory JSON parity verification
```

## Build Expectations

Phase 4D should build in these configurations:

```text
Default configuration
Explicit ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
Explicit ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

Because Phase 4D does not consume the embedded JSON at runtime, all configurations should remain behaviorally identical.

## Runtime Safety

Phase 4D remains safe because it does not introduce:

```text
Runtime JSON parsing
Disk I/O
Audio-thread JSON work
Preset source switching
Host-facing behavior changes
UI changes
MIDI changes
```

## Known Implementation Detail

The generated JUCE BinaryData header is included as:

```cpp
#include <BinaryData.h>
```

The wrapper currently expects the generated symbols:

```cpp
BinaryData::orchconductor_library_v1_example_json
BinaryData::orchconductor_library_v1_example_jsonSize
```

If JUCE generates different names in a future environment, the wrapper should be updated while preserving the public accessor API.

## Recommended Next Phase

Recommended next phase:

```text
Phase 4E: Add Embedded Factory JSON Accessor Verification
```

Suggested Phase 4E scope:

```text
Add a developer-only check that calls getEmbeddedFactoryJson().
Verify the embedded data pointer is non-null.
Verify the embedded data size is greater than zero.
Optionally parse the embedded JSON using the existing passive JSON loader.
Optionally compare embedded JSON parsing with the disk example file.
Do not modify processor/editor runtime behavior.
Do not switch preset source.
```

Phase 4E should still remain passive and developer-verification-only.

## Completion Criteria

Phase 4D is complete when:

```text
Factory JSON is included as JUCE binary data.
A passive embedded JSON accessor exists.
The plugin target builds with the binary data target linked.
Default build succeeds.
Explicit OFF build succeeds.
Explicit ON build succeeds.
JSON validation still passes.
Developer preset library check still passes.
Processor/editor behavior remains unchanged.
The phase is committed and tagged.
```

Recommended tag:

```text
phase-4D-embedded-factory-json-resource-skeleton
```
