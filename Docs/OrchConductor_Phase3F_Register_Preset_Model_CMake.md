# OrchConductor Phase 3F - Register Preset Model in CMake

## Status

Phase 3F registers the passive preset data model files from Phase 3E with the CMake build target.

## Modified Files

```text
CMakeLists.txt
```

## Added Files

```text
Docs/OrchConductor_Phase3F_Register_Preset_Model_CMake.md
```

## Purpose

Phase 3E introduced:

```text
Source/OrchConductorPresetLibrary.h
Source/OrchConductorPresetLibrary.cpp
```

The repository uses an explicit `target_sources()` block in `CMakeLists.txt`, so the new files must be listed there to participate in normal builds.

## CMake Change

The `OrchConductor` target source list now includes:

```cmake
Source/OrchConductorPresetLibrary.cpp
Source/OrchConductorPresetLibrary.h
```

## Safety Rule

Phase 3F does not change runtime behavior.

This phase only registers files with the build system.

It does not:

- Parse JSON
- Read files
- Modify MIDI output
- Modify preset switching
- Modify UI behavior
- Route runtime behavior through the new data model
- Replace hardcoded factory preset logic

## Expected Runtime Behavior

Unchanged.

The existing processor/editor code paths remain authoritative.

The new model remains passive.

## Recommended Validation

From the repository root:

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

Run the existing JSON validation script:

```powershell
.\Scripts\Validate-OrchConductorLibrary.ps1
```

Expected result:

```text
[PASS] Validation completed successfully with zero failures.
```

Then run a normal CMake configure/build, for example:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

If your local workflow uses a different build folder/configuration, use that instead.

## Phase 3F Validation Checklist

- [x] Preset model `.cpp` registered with CMake
- [x] Preset model `.h` registered with CMake
- [x] No processor logic changed
- [x] No editor logic changed
- [x] No MIDI behavior changed
- [x] No UI behavior changed
- [x] JSON validation remains available
