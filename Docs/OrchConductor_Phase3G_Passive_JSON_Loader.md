# OrchConductor Phase 3G - Passive JSON Loader

## Status

Phase 3G adds a passive JSON loading layer for the preset library data model.

## Added Files

```text
Source/OrchConductorPresetLibraryJson.h
Source/OrchConductorPresetLibraryJson.cpp
Docs/OrchConductor_Phase3G_Passive_JSON_Loader.md
```

## Modified Files

```text
CMakeLists.txt
```

## Purpose

Phase 3G introduces code that can parse the Phase 3C JSON library format into the passive C++ data model introduced in Phase 3E.

The target model is:

```cpp
orchconductor::PresetLibraryDefinition
```

The loader API is:

```cpp
orchconductor::PresetLibraryJsonLoader::fromJsonText(...)
orchconductor::PresetLibraryJsonLoader::fromJsonFile(...)
```

## JSON Mapping

The loader maps:

```text
JSON schema                          -> PresetLibraryDefinition::schema
JSON schemaVersion                   -> PresetLibraryDefinition::schemaVersion
JSON libraryName                     -> PresetLibraryDefinition::libraryName
JSON libraryVersion                  -> PresetLibraryDefinition::libraryVersion
JSON pluginTarget                    -> PresetLibraryDefinition::pluginTarget
JSON phase                           -> PresetLibraryDefinition::phase

JSON midi.channel                    -> MidiLibraryMetadata::channel
JSON midi.ccRange.min                -> MidiLibraryMetadata::ccMin
JSON midi.ccRange.max                -> MidiLibraryMetadata::ccMax
JSON midi.reservedControllers[].cc   -> ReservedControllerDefinition::ccNumber

JSON instruments[].cc                -> InstrumentDefinition::ccNumber

JSON playerProfile.overrides[].cc    -> PlayerProfileOverride::ccNumber

JSON sectionPresets.woodwinds        -> PresetLibraryDefinition::woodwindPresets
JSON sectionPresets.brass            -> PresetLibraryDefinition::brassPresets
JSON sectionPresets.percussion       -> PresetLibraryDefinition::percussionPresets
JSON sectionPresets.strings          -> PresetLibraryDefinition::stringPresets

JSON combiPresets                    -> PresetLibraryDefinition::combiPresets

JSON preset values[].cc              -> PresetValue::ccNumber
JSON preset values[].value           -> PresetValue::value
```

## Safety Rule

Phase 3G does not change runtime preset behavior.

It does not:

- Replace hardcoded presets
- Route MIDI generation through JSON data
- Read JSON automatically at plugin startup
- Modify processor behavior
- Modify editor behavior
- Modify UI behavior
- Modify preset switching behavior

## Runtime Behavior

Unchanged.

The loader is passive and only available as a utility.

## Validation

Recommended checks:

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

Run existing JSON validation:

```powershell
.\Scripts\Validate-OrchConductorLibrary.ps1
```

Expected result:

```text
[PASS] Validation completed successfully with zero failures.
```

Run CMake build:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Expected result:

```text
Build succeeds.
```

## Notes

This phase intentionally does not add runtime integration or automatic factory-library loading.

A later phase may add a controlled development-only parity check, but Phase 3G is limited to introducing and compiling the loader.
