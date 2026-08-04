# OrchConductor Phase 3D - JSON Validation Script

## Status

Phase 3D adds a developer-only PowerShell validation script for the OrchConductor JSON library files.

This phase remains non-runtime and does not modify plugin source code or MIDI behavior.

## Added Files

```text
Scripts/Validate-OrchConductorLibrary.ps1
Docs/OrchConductor_Phase3D_JSON_Validation_Script.md
```

## Purpose

The validation script provides quick project-local checks for the JSON library artifacts introduced in Phase 3A through Phase 3C.

It is intended for developer use before committing JSON library/schema changes.

## Usage

From the repository root:

```powershell
.\Scripts\Validate-OrchConductorLibrary.ps1
```

Optional custom paths:

```powershell
.\Scripts\Validate-OrchConductorLibrary.ps1 `
  -LibraryPath .\Examples\orchconductor_library_v1.example.json `
  -SchemaPath .\Schemas\orchconductor_library.schema.json
```

## Checks Performed

The script validates:

- Required files exist
- Library JSON parses
- Schema JSON parses
- Library schema identity is `orchconductor.library`
- Library schema version is `1`
- Formal schema draft is Draft 2020-12
- Formal schema constants match library identity/version
- MIDI channel is `1`
- MIDI CC range is `20-54`
- CC49 reserved controller metadata exists
- CC49 is documented as Harp/reserved/default `0`
- Instrument row count is `35`
- Harp exists at CC49 and is marked reserved
- Instrument CCs are in CC20-CC54
- Player profile defaults and string overrides are present
- Expected factory preset counts are present
- Preset CC values are in CC20-CC54
- Preset values are in 0-127
- No preset activates reserved CC49
- Expected partial values are preserved

## Expected Factory Counts

| Preset Group | Expected Count |
|---|---:|
| Woodwinds | 20 |
| Brass | 17 |
| Percussion | 9 |
| Strings | 14 |
| Combi | 29 |

## Expected Partial Values

| Preset | CC | Expected Value |
|---|---:|---:|
| `strings.low_strings` | 52 | 64 |
| `solo.english_horn_lament` | 54 | 64 |

## Non-Goals

Phase 3D does not:

- Add runtime JSON parsing
- Add schema validation inside the plugin
- Add file I/O to the plugin
- Add a library browser
- Modify C++ source
- Modify MIDI output behavior
- Replace hardcoded factory presets

## Validation Scope Note

This script is not a full JSON Schema validator.

It performs targeted structural and semantic checks that matter for the current OrchConductor factory library migration plan.

Full JSON Schema Draft 2020-12 validation can be added later through external tooling or a dedicated validation dependency.

## Phase 3D Validation Checklist

- [x] Developer validation script added
- [x] Script validates library JSON syntax
- [x] Script validates schema JSON syntax
- [x] Script validates factory preset counts
- [x] Script validates CC/value ranges
- [x] Script validates CC49 reserved behavior
- [x] Script validates known partial values
- [x] No C++ source changes
- [x] No MIDI behavior changes

## Recommended Next Phase

Recommended next step:

```text
Phase 3E: Internal Preset Data Model Preparation
```

Phase 3E can introduce internal C++ data structures representing instruments, preset values, section presets, and combi presets without adding runtime JSON loading yet.
