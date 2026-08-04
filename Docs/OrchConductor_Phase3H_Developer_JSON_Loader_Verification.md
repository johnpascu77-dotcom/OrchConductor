# OrchConductor Phase 3H - Developer JSON Loader Verification

## Status

Phase 3H adds a developer-only executable target that verifies the passive JSON loader introduced in Phase 3G.

## Added Files

```text
Tools/OrchConductorPresetLibraryCheck.cpp
Docs/OrchConductor_Phase3H_Developer_JSON_Loader_Verification.md
```

## Modified Files

```text
CMakeLists.txt
```

## Purpose

The verification target loads the factory JSON example:

```text
Examples/orchconductor_library_v1.example.json
```

Then it parses the file through:

```cpp
orchconductor::PresetLibraryJsonLoader::fromJsonFile(...)
```

and verifies that the resulting passive model has the expected metadata and preset counts.

## Target Name

```text
OrchConductorPresetLibraryCheck
```

## Checks

The tool verifies:

```text
schema:        orchconductor.library
schemaVersion: 1
libraryName:   OrchConductor Factory Library
pluginTarget:  OrchConductor

midi.channel:  1
midi.ccMin:    20
midi.ccMax:    54

reserved controllers: 1
reserved CC:          49
reserved name:        Harp
reserved status:      reserved
reserved default:     0

woodwinds:     20
brass:         17
percussion:    9
strings:       14
combis:        29
```

The tool also calls:

```cpp
library.isValid()
```

## Safety Rule

Phase 3H does not change plugin runtime behavior.

It does not:

- Modify processor behavior
- Modify editor behavior
- Modify MIDI output
- Modify preset switching
- Load JSON from the plugin at runtime
- Replace hardcoded preset behavior

The check target is a separate developer executable.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Debug --target OrchConductorPresetLibraryCheck
```

## Run

Depending on the Visual Studio generator output layout, try:

```powershell
.\build\Debug\OrchConductorPresetLibraryCheck.exe
```

If not found, locate it with:

```powershell
Get-ChildItem .\build -Recurse -Filter OrchConductorPresetLibraryCheck.exe
```

Then run the discovered path.

## Expected Result

```text
[PASS] Passive JSON loader verification completed successfully.
```

## Existing Validation

The PowerShell JSON validation script remains authoritative for schema/project-level JSON validation:

```powershell
.\Scripts\Validate-OrchConductorLibrary.ps1
```

Expected result:

```text
[PASS] Validation completed successfully with zero failures.
```
