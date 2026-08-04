# OrchConductor Phase 4L - Runtime Preset Catalog Adapter Parity Verification

## Status

Phase 4L extends the read-only runtime preset catalog adapter with passive factory-shape metadata verification.

The adapter remains non-authoritative.

Hardcoded factory presets remain authoritative.

No editor behavior changes.

No processor behavior changes.

No MIDI behavior changes.

## Modified Files

```text
Source/OrchConductorRuntimePresetCatalog.h
Source/OrchConductorRuntimePresetCatalog.cpp
Tools/OrchConductorRuntimePresetCatalogCheck.cpp
```

## Added Files

```text
Docs/OrchConductor_Phase4L_Runtime_Preset_Catalog_Adapter_Parity_Verification.md
```

## Purpose

Phase 4L verifies that the catalog adapter can safely carry minimal factory catalog shape metadata while preserving fallback-first behavior.

This is a stepping stone toward later read-only metadata access.

It does not adopt JSON-backed behavior.

## Added Passive Accessors

```cpp
int getSectionCount() const noexcept;
int getCombiPresetCount() const noexcept;
bool hasExpectedFactoryShape() const noexcept;
```

These accessors are intentionally limited.

They do not expose:

```text
Preset labels.
Preset values.
CC mappings.
MIDI event order.
Editor combo data.
State serialization IDs.
```

## Catalog Construction Rules

Fallback catalog construction:

```text
Creates a ready catalog.
Requires fallback.
Stores fallback factory-shape metadata with expected factory counts.
Provides non-empty diagnostics.
```

Runtime-source catalog construction:

```text
If runtime source is loaded and does not require fallback:
    Creates a ready catalog.
    Does not require fallback.
    Stores runtime source library definition.
    Provides non-empty diagnostics.

Otherwise:
    Creates a ready catalog.
    Requires fallback.
    Stores fallback factory-shape metadata with expected factory counts.
    Provides non-empty diagnostics.
```

## Expected Factory Shape

The developer check verifies:

```text
Section count: 5
Woodwind preset count: 20
Brass preset count: 17
Percussion preset count: 9
String preset count: 14
Combi preset count: 29
Library is valid
```

This confirms the adapter can carry the same top-level factory shape as the hardcoded and JSON-loaded libraries.

## Important Limitation

Phase 4L does not create a full hardcoded factory library model.

The fallback catalog stores only valid factory-shape metadata sufficient for passive shape verification.

It does not store authoritative labels or MIDI values.

Hardcoded processor/editor paths remain the only authoritative fallback behavior.

## OFF Build Expectations

With:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
```

the catalog check verifies:

```text
Fallback catalog is ready.
Fallback catalog requires fallback.
Fallback catalog has expected factory shape.
Runtime source does not load JSON.
Runtime-source catalog is ready.
Runtime-source catalog requires fallback.
Runtime-source catalog has expected factory shape via fallback factory-shape metadata.
Diagnostics are non-empty.
```

Expected final lines:

```text
[PASS] Runtime preset catalog adapter parity verification completed successfully with runtime JSON OFF.
[PASS] Catalog adapter remains non-authoritative.
```

## ON Build Expectations

With:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

the catalog check verifies:

```text
Fallback catalog is ready.
Fallback catalog requires fallback.
Fallback catalog has expected factory shape.
Runtime source loads embedded JSON.
Runtime-source catalog is ready.
Runtime-source catalog does not require fallback.
Runtime-source catalog has expected factory shape via runtime source.
Diagnostics are non-empty.
```

Expected final lines:

```text
[PASS] Runtime preset catalog adapter parity verification completed successfully with runtime JSON ON.
[PASS] Catalog adapter remains non-authoritative.
```

## Non-Adoption Guarantee

Phase 4L does not:

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
Make JSON authoritative.
```

## Completion Criteria

Phase 4L is complete when:

```text
Passive catalog metadata accessors are added.
Catalog adapter check verifies fallback shape.
Catalog adapter check verifies runtime-source shape.
Default/OFF catalog check passes.
Explicit OFF catalog check passes.
Explicit ON catalog check passes.
Existing developer checks still build.
Plugin target still builds.
CMakeLists.txt remains untouched.
Processor/editor/runtime-source implementation files remain untouched.
Working tree is clean after commit.
Phase is tagged.
```

Recommended commit message:

```text
Add runtime preset catalog adapter parity verification
```

Recommended tag:

```text
phase-4L-runtime-preset-catalog-adapter-parity-verification
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4M: Runtime Preset Catalog Label Access Trial
```

Suggested scope:

```text
Add read-only section and preset label accessors to the catalog adapter.
Keep hardcoded authoritative.
Add developer-only label parity checks.
Do not change editor population.
Do not change processBlock().
Do not change MIDI output.
```
