# OrchConductor Phase 4E - Embedded Factory JSON Accessor Verification

## Status

Phase 4E adds a developer-only verification target for the embedded factory JSON accessor introduced in Phase 4D.

Phase 4E remains passive.

Phase 4E does not change plugin runtime behavior.

Phase 4E does not change preset behavior.

Phase 4E does not change MIDI behavior.

Phase 4E does not change UI behavior.

## Modified Files

```text
CMakeLists.txt
```

## Added Files

```text
Tools/OrchConductorEmbeddedFactoryJsonCheck.cpp
Docs/OrchConductor_Phase4E_Embedded_Factory_JSON_Accessor_Verification.md
```

## Verification Target

Phase 4E adds the developer-only executable:

```text
OrchConductorEmbeddedFactoryJsonCheck
```

The target links:

```text
juce::juce_core
OrchConductorFactoryJsonData
```

The target compiles with:

```text
Source/OrchConductorPresetLibrary.cpp
Source/OrchConductorPresetLibraryJson.cpp
Source/OrchConductorEmbeddedFactoryJson.cpp
```

## What The Check Verifies

The embedded factory JSON check verifies:

```text
getEmbeddedFactoryJson() returns a non-null pointer
getEmbeddedFactoryJson() returns a size greater than zero
EmbeddedFactoryJson::isValid() returns true
Embedded pointer/size data can be converted to juce::String text
Embedded JSON text parses through PresetLibraryJsonLoader::fromJsonText()
Parsed metadata matches expected factory values
Parsed MIDI metadata matches expected factory values
Parsed factory preset counts match expected domain counts
Reserved CC49 policy is still present
Parsed library passes final isValid()
```

Expected factory counts:

```text
Woodwinds: 20
Brass: 17
Percussion: 9
Strings: 14
Combi: 29
```

Expected MIDI policy:

```text
Channel: 1
CC min: 20
CC max: 54
Reserved CC: 49
```

## What Phase 4E Does Not Do

Phase 4E does not:

```text
Parse embedded JSON inside plugin runtime
Use embedded JSON during plugin startup
Switch factory preset source
Modify processor behavior
Modify editor behavior
Emit MIDI from JSON-backed data
Modify UI labels or preset ordering
Add fallback runtime logic
Add audio-thread JSON work
Add disk-loaded factory behavior
```

## Runtime Safety

Phase 4E remains runtime-safe because the new embedded JSON check is a developer-only executable.

The plugin runtime remains unchanged.

The following files are intentionally not modified:

```text
Source/OrchConductorProcessor.cpp
Source/OrchConductorEditor.cpp
```

## Compile Gate Relationship

Phase 4E does not activate runtime JSON behavior.

The developer-only check may be built regardless of:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

The feature gate continues to control future runtime JSON-backed preset infrastructure.

## Expected Output

Successful verification should end with:

```text
[PASS] Embedded factory JSON accessor verification completed successfully.
[PASS] Embedded factory JSON passive loader verification completed successfully.
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4F: Runtime Embedded JSON Load Attempt Behind Feature Gate
```

Suggested Phase 4F scope:

```text
Add construction-time embedded JSON load attempt behind ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS.
Keep hardcoded presets as fallback authority.
Do not switch runtime preset source yet unless validation succeeds and behavior remains unchanged.
Avoid audio-thread JSON parsing.
Avoid UI/MIDI behavior changes on failure.
Document fallback behavior.
```

A safer alternative is:

```text
Phase 4F: Runtime JSON Loader Boundary Skeleton
```

That alternative would add a passive runtime-facing loader class without connecting it to processor/editor behavior yet.

## Completion Criteria

Phase 4E is complete when:

```text
Developer-only embedded JSON check target exists.
Embedded accessor pointer/size are verified.
Embedded JSON parses through the passive JSON loader.
Core factory metadata and counts are verified.
Final library isValid() check passes.
Default build succeeds.
Explicit OFF build succeeds.
Explicit ON build succeeds.
Existing disk JSON loader check still passes.
Processor/editor files remain unchanged.
The phase is committed and tagged.
```

Recommended tag:

```text
phase-4E-embedded-factory-json-accessor-verification
```
