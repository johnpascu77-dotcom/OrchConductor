# OrchConductor Phase 4N - Runtime Preset Catalog Label Parity Verification

## Status

Phase 4N adds explicit factory label snapshot verification to the runtime preset catalog developer check.

The catalog adapter remains non-authoritative.

Hardcoded factory presets remain authoritative.

No editor behavior changes.

No processor behavior changes.

No MIDI behavior changes.

## Modified Files

```text
Tools/OrchConductorRuntimePresetCatalogCheck.cpp
```

## Added Files

```text
Docs/OrchConductor_Phase4N_Runtime_Preset_Catalog_Label_Parity_Verification.md
```

## Purpose

Phase 4N verifies that runtime-source catalog labels match an explicit factory label snapshot when runtime JSON presets are enabled.

This phase strengthens the Phase 4M label access trial by checking known first and last labels for each factory group.

It does not make the catalog authoritative.

It does not connect the catalog to the editor.

It does not connect the catalog to MIDI generation.

## Explicit Factory Label Snapshot

The ON-build developer check verifies:

```text
woodwinds first:   All Off
woodwinds last:    Full Woodwinds
brass first:       All Off
brass last:        Full Brass
percussion first:  All Off
percussion last:   Full Melodic Percussion
strings first:     All Off
strings last:      Tutti
combi first:       Manual Sections
combi last:        [Solo] English Horn Lament
```

These labels were extracted from:

```text
Examples/orchconductor_library_v1.example.json
```

## OFF Build Behavior

With:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
```

the catalog check verifies:

```text
Fallback catalog is ready.
Fallback catalog requires fallback.
Fallback catalog has expected factory shape.
Fallback label access is safe.
Fallback labels are non-empty placeholders.
Runtime source does not load JSON.
Runtime-source catalog requires fallback.
Runtime-source catalog has expected factory shape through fallback metadata.
Runtime-source catalog labels are safe fallback placeholders.
Fallback labels do not claim real factory label parity.
Invalid label indexes return empty strings.
Unknown section IDs return empty labels and zero counts.
```

Expected final lines:

```text
[PASS] Runtime preset catalog label parity verification completed successfully with runtime JSON OFF.
[PASS] Catalog label parity remains non-authoritative.
```

## ON Build Behavior

With:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

the catalog check verifies:

```text
Fallback catalog remains available.
Fallback catalog requires fallback.
Runtime source loads embedded JSON.
Runtime-source catalog does not require fallback.
Runtime-source catalog has expected factory shape.
Runtime-source catalog labels mirror the loaded runtime source labels.
Runtime-source catalog labels match explicit expected factory label snapshots.
Invalid label indexes return empty strings.
Unknown section IDs return empty labels and zero counts.
```

Expected final lines:

```text
[PASS] Runtime preset catalog label parity verification completed successfully with runtime JSON ON.
[PASS] Catalog label parity remains non-authoritative.
```

## Non-Adoption Guarantee

Phase 4N does not:

```text
Change catalog adapter public API.
Change catalog adapter implementation.
Change editor labels.
Change combo-box population.
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

Phase 4N is complete when:

```text
Runtime preset catalog check verifies explicit ON label snapshots.
Runtime preset catalog check verifies OFF fallback labels do not claim real label parity.
Default/OFF catalog check passes.
Explicit OFF catalog check passes.
Explicit ON catalog check passes.
Existing developer checks still build.
Plugin target still builds.
CMakeLists.txt remains untouched.
Catalog adapter source/header remain untouched.
Processor/editor/runtime-source files remain untouched.
Examples/Schemas/Scripts remain untouched.
Working tree is clean after commit.
Phase is tagged.
```

Recommended commit message:

```text
Add runtime preset catalog label parity verification
```

Recommended tag:

```text
phase-4N-runtime-preset-catalog-label-parity-verification
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4O: Runtime Preset Catalog Value Access Trial
```

Suggested scope:

```text
Add read-only preset value accessors to the catalog adapter.
Verify safe access only in developer checks.
Keep hardcoded processor/editor paths authoritative.
Do not change editor population.
Do not change processBlock().
Do not change MIDI output.
```
