# OrchConductor Phase 4K - Read-only Runtime Preset Catalog Adapter Skeleton

## Status

Phase 4K adds a read-only runtime preset catalog adapter skeleton.

The adapter is non-authoritative.

The adapter does not change plugin behavior.

Hardcoded factory presets remain authoritative.

Runtime JSON preset adoption remains disabled.

## Added Files

```text
Source/OrchConductorRuntimePresetCatalog.h
Source/OrchConductorRuntimePresetCatalog.cpp
Tools/OrchConductorRuntimePresetCatalogCheck.cpp
Docs/OrchConductor_Phase4K_Read_Only_Runtime_Preset_Catalog_Adapter_Skeleton.md
```

## Modified Files

```text
CMakeLists.txt
```

## Intent

Phase 4K creates a small boundary object that can later represent a validated runtime preset catalog.

In this phase, it exposes only:

```text
Readiness state.
Fallback-required state.
Diagnostic message.
```

It does not expose:

```text
Preset names.
Preset values.
Section data.
Combi data.
MIDI data.
Editor data.
Plugin state data.
```

## Public API

```cpp
static OrchConductorRuntimePresetCatalog createFallbackCatalog();
static OrchConductorRuntimePresetCatalog createFromRuntimeSource(const OrchConductorRuntimePresetSource& source);

bool isReady() const;
bool requiresFallback() const;
juce::String getDiagnosticMessage() const;
```

## Safety Boundaries

The catalog adapter:

```text
Does not parse JSON.
Does not access disk.
Does not access embedded binary data directly.
Does not emit MIDI.
Does not mutate plugin state.
Does not populate the editor.
Does not run in processBlock().
Does not become authoritative for preset behavior.
```

`RuntimePresetSource` remains responsible for embedded JSON loading and validation.

The catalog adapter only reflects source readiness and fallback diagnostics.

## OFF Build Behavior

When built with:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
```

expected behavior is:

```text
Fallback catalog is ready.
Fallback catalog requires fallback.
Runtime source does not load JSON.
Runtime-source catalog is ready.
Runtime-source catalog requires fallback.
Diagnostics are non-empty.
```

Expected final lines:

```text
[PASS] Read-only runtime preset catalog adapter verification completed successfully with runtime JSON OFF.
[PASS] Catalog adapter remains non-authoritative.
```

## ON Build Behavior

When built with:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

expected behavior is:

```text
Fallback catalog is ready.
Fallback catalog requires fallback.
Runtime source loads embedded JSON.
Runtime-source catalog is ready.
Runtime-source catalog does not require fallback.
Diagnostics are non-empty.
```

Expected final lines:

```text
[PASS] Read-only runtime preset catalog adapter verification completed successfully with runtime JSON ON.
[PASS] Catalog adapter remains non-authoritative.
```

## Non-Adoption Guarantee

Phase 4K intentionally does not:

```text
Change editor labels.
Change preset selection.
Change processBlock().
Change MIDI output.
Change emitted CC count.
Change MIDI channel.
Change reserved CC49 behavior.
Change state serialization.
Change hardcoded preset library behavior.
```

## Completion Criteria

Phase 4K is complete when:

```text
Catalog adapter files are added.
Developer catalog check target is added.
Default/OFF catalog check passes.
Explicit OFF catalog check passes.
Explicit ON catalog check passes.
Existing developer checks still build.
Plugin target still builds.
Processor/editor/runtime-source implementation files remain untouched.
Working tree is clean after commit.
Phase is tagged.
```

Recommended tag:

```text
phase-4K-read-only-runtime-preset-catalog-adapter-skeleton
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4L: Runtime Preset Catalog Adapter Parity Verification
```

Suggested scope:

```text
Extend the adapter toward read-only section/combi metadata access.
Keep hardcoded authoritative.
Add developer-only parity checks.
Do not change editor population.
Do not change processBlock().
Do not change MIDI output.
```
