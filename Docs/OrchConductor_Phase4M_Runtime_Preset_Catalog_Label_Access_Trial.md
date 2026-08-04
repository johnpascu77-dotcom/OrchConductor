# OrchConductor Phase 4M - Runtime Preset Catalog Label Access Trial

## Status

Phase 4M adds passive read-only label accessors to the runtime preset catalog adapter.

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
Docs/OrchConductor_Phase4M_Runtime_Preset_Catalog_Label_Access_Trial.md
```

## Purpose

Phase 4M verifies that the catalog adapter can safely expose read-only preset labels without affecting plugin behavior.

This is a trial only.

The editor does not consume these labels.

The processor does not consume these labels.

MIDI output does not consume these labels.

## Added Accessors

```cpp
int getSectionPresetCount(const juce::String& sectionId) const noexcept;
juce::String getSectionPresetLabel(const juce::String& sectionId, int presetIndex) const;
juce::String getCombiPresetLabel(int presetIndex) const;
```

Existing Phase 4L accessors remain:

```cpp
int getSectionCount() const noexcept;
int getCombiPresetCount() const noexcept;
bool hasExpectedFactoryShape() const noexcept;
```

## Supported Section IDs

```text
woodwinds
brass
percussion
strings
```

Unknown section IDs return empty labels and zero preset count.

Out-of-range indexes return empty labels.

Negative indexes return empty labels.

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
Fallback labels are non-empty placeholders.
Runtime source does not load JSON.
Runtime-source catalog requires fallback.
Runtime-source catalog has expected factory shape through fallback metadata.
Runtime-source catalog labels are safe fallback placeholders.
Invalid label indexes return empty strings.
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
Invalid label indexes return empty strings.
```

## Non-Adoption Guarantee

Phase 4M does not:

```text
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

Phase 4M is complete when:

```text
Passive label accessors are added.
Catalog adapter check verifies safe fallback labels.
Catalog adapter check verifies ON runtime-source labels mirror loaded source labels.
Default/OFF catalog check passes.
Explicit OFF catalog check passes.
Explicit ON catalog check passes.
Existing developer checks still build.
Plugin target still builds.
CMakeLists.txt remains untouched.
Processor/editor/runtime-source files remain untouched.
Working tree is clean after commit.
Phase is tagged.
```

Recommended commit message:

```text
Add runtime preset catalog label access trial
```

Recommended tag:

```text
phase-4M-runtime-preset-catalog-label-access-trial
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4N: Runtime Preset Catalog Label Parity Verification
```

Suggested scope:

```text
Verify runtime-source labels against explicit expected factory label snapshots.
Keep catalog non-authoritative.
Do not change editor population.
Do not change processBlock().
Do not change MIDI output.
```
