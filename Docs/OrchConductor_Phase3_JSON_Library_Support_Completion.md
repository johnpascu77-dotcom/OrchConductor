\# OrchConductor Phase 3 - JSON Library Support Completion



\## Status



Phase 3 is complete.



Phase 3 established a JSON-backed factory preset library foundation while preserving the existing hardcoded runtime behavior.



The JSON library, schema, validation tooling, passive C++ data model, passive JSON loader, and developer verification target are now present on `main`.



\## Phase 3 Milestones



\### Phase 3A - JSON Schema Direction



Added initial documentation and example direction for externalizing factory preset data into a JSON library format.



Tag:



```text

phase-3A-json-schema

```



\### Phase 3B - Formal JSON Schema



Added the formal machine-readable schema:



```text

Schemas/orchconductor\_library.schema.json

```



Tag:



```text

phase-3B-formal-json-schema

```



\### Phase 3C - Complete Factory JSON Library



Expanded the example JSON into a full factory mirror:



```text

Examples/orchconductor\_library\_v1.example.json

```



The JSON now covers:



```text

Woodwinds presets:  20

Brass presets:      17

Percussion presets: 9

Strings presets:    14

Combi presets:      29

```



Tag:



```text

phase-3C-complete-factory-json

```



\### Phase 3D - JSON Validation Script



Added developer validation tooling:



```text

Scripts/Validate-OrchConductorLibrary.ps1

```



The script validates schema/project constraints and confirms known factory rules.



Tag:



```text

phase-3D-json-validation-script

```



\### Phase 3E - Passive Internal Data Model



Added passive C++ structures representing the JSON library:



```text

Source/OrchConductorPresetLibrary.h

Source/OrchConductorPresetLibrary.cpp

```



Primary model:



```cpp

orchconductor::PresetLibraryDefinition

```



The model includes validation helpers through `isValid()` methods.



Tag:



```text

phase-3E-internal-preset-data-model

```



\### Phase 3F - CMake Registration



Registered the passive preset library model with the build system:



```text

CMakeLists.txt

```



Tag:



```text

phase-3F-register-preset-model-cmake

```



\### Phase 3G - Passive JSON Loader



Added a passive JSON loader:



```text

Source/OrchConductorPresetLibraryJson.h

Source/OrchConductorPresetLibraryJson.cpp

```



Primary API:



```cpp

orchconductor::PresetLibraryJsonLoader::fromJsonText(...)

orchconductor::PresetLibraryJsonLoader::fromJsonFile(...)

```



Tag:



```text

phase-3G-passive-json-loader

```



\### Phase 3H - Developer JSON Loader Verification



Added a developer-only verification target:



```text

OrchConductorPresetLibraryCheck

```



Source:



```text

Tools/OrchConductorPresetLibraryCheck.cpp

Tools/JuceHeader.h

```



This target loads:



```text

Examples/orchconductor\_library\_v1.example.json

```



and verifies that the passive loader produces the expected model shape and counts.



Tag:



```text

phase-3H-developer-json-loader-verification

```



\### Phase 3I - Factory JSON Parity Verification



Enhanced the developer verification target with factory parity checks.



It verifies:



```text

Sequential factory IDs

Unique preset IDs within each domain

Section/domain consistency

Configured CC range usage

Reserved CC49 is not targeted by preset values

Value range 0..127

Known all-off/manual entries

Special partial-value behavior

```



Special partial values verified:



```text

strings.low\_strings contains CC52 = 64

solo.english\_horn\_lament contains CC54 = 64

```



Tag:



```text

phase-3I-factory-json-parity-verification

```



\## Current Phase 3 Assets



\### JSON Library



```text

Examples/orchconductor\_library\_v1.example.json

```



\### JSON Schema



```text

Schemas/orchconductor\_library.schema.json

```



\### Validation Script



```text

Scripts/Validate-OrchConductorLibrary.ps1

```



\### Passive C++ Model



```text

Source/OrchConductorPresetLibrary.h

Source/OrchConductorPresetLibrary.cpp

```



\### Passive JSON Loader



```text

Source/OrchConductorPresetLibraryJson.h

Source/OrchConductorPresetLibraryJson.cpp

```



\### Developer Verification Target



```text

Tools/OrchConductorPresetLibraryCheck.cpp

Tools/JuceHeader.h

```



\### Phase Documentation



```text

Docs/OrchConductor\_Phase3G\_Passive\_JSON\_Loader.md

Docs/OrchConductor\_Phase3H\_Developer\_JSON\_Loader\_Verification.md

Docs/OrchConductor\_Phase3I\_Factory\_JSON\_Parity\_Verification.md

Docs/OrchConductor\_Phase3\_JSON\_Library\_Support\_Completion.md

```



\## Factory Library Shape



Top-level JSON keys:



```text

schema

schemaVersion

libraryName

libraryVersion

pluginTarget

phase

notes

midi

instruments

playerProfile

sectionPresets

combiPresets

```



Preset containers:



```text

sectionPresets.woodwinds

sectionPresets.brass

sectionPresets.percussion

sectionPresets.strings

combiPresets

```



Expected counts:



```text

woodwinds:  20

brass:      17

percussion: 9

strings:    14

combis:     29

```



\## MIDI and CC Rules



The Phase 3 library preserves the current factory MIDI control layout.



Configured CC range:



```text

CC20..CC54

```



Reserved controller:



```text

CC49 Harp

```



Important rules:



```text

Preset values must use CC20..CC54.

Preset values must not target CC49.

Preset values must be 0..127.

Reserved CC49 is represented only in metadata.

```



Special partial-value behavior preserved:



```text

strings.low\_strings       CC52 = 64

solo.english\_horn\_lament  CC54 = 64

```



\## Runtime Integration Boundary



Phase 3 intentionally does not change runtime preset behavior.



The following remain authoritative for runtime behavior at the end of Phase 3:



```text

Existing processor/editor code paths

Existing hardcoded preset selection behavior

Existing MIDI CC generation behavior

Existing UI behavior

```



Phase 3 does not:



```text

Load JSON automatically at plugin startup

Replace hardcoded factory presets

Route MIDI generation through JSON-loaded data

Change preset switching behavior

Change plugin UI behavior

Change host-facing behavior

```



The JSON loader is passive.



The developer check target is separate from the plugin runtime.



\## Files That Should Remain Untouched by Phase 3 Runtime Work



The Phase 3 JSON-library work avoided behavioral edits to:



```text

Source/OrchConductorProcessor.cpp

Source/OrchConductorEditor.cpp

Source/PluginProcessor.cpp

Source/PluginEditor.cpp

```



Before beginning runtime integration, confirm there are no unintended changes:



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



\## Validation Commands



\### PowerShell JSON Validation



```powershell

.\\Scripts\\Validate-OrchConductorLibrary.ps1

```



Expected result:



```text

\[PASS] Validation completed successfully with zero failures.

```



\### Configure



```powershell

cmake -S . -B build

```



\### Build Developer Verification Target



```powershell

cmake --build build --config Debug --target OrchConductorPresetLibraryCheck

```



\### Locate Verification Executable



```powershell

Get-ChildItem .\\build -Recurse -Filter OrchConductorPresetLibraryCheck.exe

```



\### Run Verification Executable



Example:



```powershell

.\\build\\Debug\\OrchConductorPresetLibraryCheck.exe

```



Expected final lines:



```text

\[PASS] Passive JSON loader verification completed successfully.

\[PASS] Factory JSON parity verification completed successfully.

```



\## Recommended Pre-Phase-4 Checklist



Before beginning any runtime JSON integration work:



```text

1\. Confirm main is clean.

2\. Run PowerShell JSON validation.

3\. Build the plugin.

4\. Build OrchConductorPresetLibraryCheck.

5\. Run OrchConductorPresetLibraryCheck.

6\. Confirm processor/editor files are unchanged.

7\. Write a Phase 4 integration plan before runtime code changes.

```



Recommended commands:



```powershell

git status --short

.\\Scripts\\Validate-OrchConductorLibrary.ps1

cmake -S . -B build

cmake --build build --config Debug

cmake --build build --config Debug --target OrchConductorPresetLibraryCheck

Get-ChildItem .\\build -Recurse -Filter OrchConductorPresetLibraryCheck.exe

```



\## Recommended Phase 4 Direction



Phase 4 should begin with planning, not immediate runtime behavior changes.



Recommended next document:



```text

Docs/OrchConductor\_Phase4\_Runtime\_JSON\_Integration\_Plan.md

```



The Phase 4 plan should define:



```text

Feature flag or build option strategy

Fallback to hardcoded factory presets

Error handling if JSON load fails

Parity verification before enabling runtime use

Whether factory JSON is embedded or loaded from disk

Host-safe startup behavior

Testing requirements

Rollback strategy

```



\## Completion Statement



Phase 3 successfully externalized the factory preset library into a validated JSON format and introduced passive C++ infrastructure for loading and checking that data.



At completion, OrchConductor has a verified JSON library foundation while preserving existing runtime behavior.



