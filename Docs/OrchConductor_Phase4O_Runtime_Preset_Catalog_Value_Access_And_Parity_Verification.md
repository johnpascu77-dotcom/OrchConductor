# OrchConductor Phase 4O - Runtime Preset Catalog Value Access and Parity Verification

## Status

Phase 4O adds typed, read-only preset value access to the runtime preset catalog adapter.

The catalog remains non-authoritative. Hardcoded factory presets remain authoritative.

## Modified Files

```text
Source/OrchConductorRuntimePresetCatalog.h
Source/OrchConductorRuntimePresetCatalog.cpp
Tools/OrchConductorRuntimePresetCatalogCheck.cpp
```

## Added Files

```text
Docs/OrchConductor_Phase4O_Runtime_Preset_Catalog_Value_Access_And_Parity_Verification.md
```

## Added API

```cpp
struct OrchConductorRuntimePresetValueView
{
    int ccNumber = -1;
    int value = -1;
    bool isValid = false;
};

int getSectionPresetValueCount(const juce::String& sectionId, int presetIndex) const noexcept;
int getCombiPresetValueCount(int presetIndex) const noexcept;

OrchConductorRuntimePresetValueView getSectionPresetValue(const juce::String& sectionId,
                                                          int presetIndex,
                                                          int valueIndex) const noexcept;

OrchConductorRuntimePresetValueView getCombiPresetValue(int presetIndex,
                                                        int valueIndex) const noexcept;
```

Invalid access returns zero counts or invalid typed views.

## Verification

The developer catalog check verifies:

```text
Fallback value access is safe.
Invalid section/preset/value indexes are safe.
Unknown section value access is safe.
Runtime-source value counts mirror loaded source values when JSON is ON.
Sampled runtime-source values mirror source CC/value pairs when JSON is ON.
Runtime-source catalog remains fallback-only when JSON is OFF.
```

## Non-Adoption Guarantee

Phase 4O does not change:

```text
Editor population
Preset selection
processBlock()
MIDI output
State serialization
CMake configuration
Runtime source loading behavior
```

The runtime catalog still does not drive MIDI or UI behavior.

## Recommended Commit

```text
Add runtime preset catalog value access verification
```

## Recommended Tag

```text
phase-4O-runtime-preset-catalog-value-access-parity-verification
```
