# OrchConductor Phase 3E - Internal Preset Data Model

## Status

Phase 3E introduces passive C++ data structures that mirror the JSON library shape introduced in Phase 3A through Phase 3D.

This phase prepares for future runtime JSON loading, but does not perform JSON parsing yet.

## Added Files

```text
Source/OrchConductorPresetLibrary.h
Source/OrchConductorPresetLibrary.cpp
Docs/OrchConductor_Phase3E_Internal_Preset_Data_Model.md
```

## Purpose

The new internal data model provides C++ representations for:

- Preset values
- Instrument definitions
- Player profile overrides
- Player profiles
- Section presets
- Combi presets
- Reserved MIDI controllers
- MIDI library metadata
- Complete preset library definitions

## Important Safety Rule

Phase 3E does not change runtime behavior.

The new model is passive and is not connected to:

- MIDI CC output
- Section preset switching
- Combi preset switching
- UI state synchronization
- Active player calculations
- File I/O
- JSON parsing

The existing hardcoded C++ preset logic remains authoritative at runtime.

## Added Types

Namespace:

```cpp
namespace orchconductor
```

Types:

```cpp
PresetValue
InstrumentDefinition
PlayerProfileOverride
PlayerProfile
SectionPresetDefinition
CombiPresetDefinition
ReservedControllerDefinition
MidiLibraryMetadata
PresetLibraryDefinition
```

## Validation Helpers

Each type includes a lightweight `isValid()` helper.

These helpers only validate basic internal consistency, such as:

- CC numbers are within `0-127`
- MIDI values are within `0-127`
- MIDI channel is within `1-16`
- required strings are non-empty
- schema identity is `orchconductor.library`
- schema version is `1`

## Relationship to JSON

The data model mirrors the current JSON concepts, but is not yet populated from JSON.

Future phases can add:

- JSON parsing
- conversion from `juce::var` or `juce::DynamicObject`
- factory library construction
- parity tests between JSON and hardcoded logic

## Non-Goals

Phase 3E does not:

- Parse JSON
- Read files
- Add a library browser
- Replace hardcoded presets
- Change the plugin processor logic
- Change MIDI output behavior
- Change UI behavior
- Change preset names
- Change active player metadata

## Validation

Recommended checks after applying Phase 3E:

```powershell
git diff -- Source/OrchConductorProcessor.cpp
git diff -- Source/OrchConductorEditor.cpp
git diff -- Source/PluginProcessor.cpp
git diff -- Source/PluginEditor.cpp
```

Expected result:

```text
No output
```

Also run a normal build to confirm the new files compile if they are included by the project generator or build system.

## Phase 3E Validation Checklist

- [x] Passive C++ preset data model added
- [x] No JSON parsing added
- [x] No file I/O added
- [x] No runtime preset routing changed
- [x] No MIDI output behavior changed
- [x] No UI behavior changed
- [x] Existing hardcoded factory logic remains authoritative
