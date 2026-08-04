# OrchConductor Phase 3I - Factory JSON Parity Verification

## Status

Phase 3I enhances the developer-only preset library check target introduced in Phase 3H.

## Modified Files

```text
Tools/OrchConductorPresetLibraryCheck.cpp
```

## Added Files

```text
Docs/OrchConductor_Phase3I_Factory_JSON_Parity_Verification.md
```

## Purpose

Phase 3H proved that the passive JSON loader can parse the factory JSON file.

Phase 3I adds parity-oriented checks that verify the loaded JSON library preserves expected factory-library meaning.

The check target remains:

```text
OrchConductorPresetLibraryCheck
```

## Added Verification

Phase 3I verifies:

```text
All section preset factory IDs are sequential.
All combi preset factory IDs are sequential.
All preset IDs are non-empty.
All preset IDs are unique within each domain.
All section preset .section values match their containing section.
All preset values use configured CC range 20..54.
No preset value targets reserved CC49.
All preset values are in 0..127.
```

It also verifies known factory entries:

```text
woodwinds.all_off
brass.all_off
percussion.all_off
strings.all_off
manual.sections
```

And special partial-value behavior:

```text
strings.low_strings contains CC52 = 64
solo.english_horn_lament contains CC54 = 64
```

Reserved controller policy is verified:

```text
reserved controller count: 1
reserved CC:               49
reserved name:             Harp
reserved status:           reserved
reserved default value:    0
```

## Safety Rule

Phase 3I does not change plugin runtime behavior.

It does not:

- Modify processor behavior
- Modify editor behavior
- Modify MIDI output
- Modify preset switching
- Load JSON from the plugin at runtime
- Replace hardcoded preset behavior

The verification remains isolated in a developer executable.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Debug --target OrchConductorPresetLibraryCheck
```

## Run

Locate the executable if needed:

```powershell
Get-ChildItem .\build -Recurse -Filter OrchConductorPresetLibraryCheck.exe
```

Then run it.

Example:

```powershell
.\build\Debug\OrchConductorPresetLibraryCheck.exe
```

## Expected Result

```text
[PASS] Passive JSON loader verification completed successfully.
[PASS] Factory JSON parity verification completed successfully.
```

## Existing JSON Validation

The schema/project-level validation script should still pass:

```powershell
.\Scripts\Validate-OrchConductorLibrary.ps1
```

Expected result:

```text
[PASS] Validation completed successfully with zero failures.
```
