# OrchConductor Phase 3B - Formal JSON Schema File

## Status

Phase 3B adds a formal JSON Schema document for the Phase 3A OrchConductor library format.

This remains a non-runtime documentation and validation checkpoint.

## Added File

```text
Schemas/orchconductor_library.schema.json
```

## Scope

Phase 3B defines machine-readable validation rules for:

- Top-level library object structure
- Schema identity and schema version
- MIDI CC range
- Reserved controller metadata
- Instrument definitions
- Active player metadata profile
- Section presets
- Combi presets
- Preset CC/value pairs

## Non-Goals

Phase 3B does not:

- Add JSON parsing to the plugin
- Add runtime file loading
- Add schema validation inside the plugin
- Modify C++ source
- Modify MIDI output behavior
- Modify existing hardcoded presets
- Modify active player metadata behavior

## Schema Version

The formal schema currently targets:

```json
{
  "schema": "orchconductor.library",
  "schemaVersion": 1
}
```

## Important Constraints

The schema currently enforces:

| Constraint | Value |
|---|---|
| MIDI CC minimum | 20 |
| MIDI CC maximum | 54 |
| MIDI channel range | 1-16 |
| Preset value range | 0-127 |
| Schema version | 1 |
| Schema name | `orchconductor.library` |

## Reserved CC49 Note

The schema allows CC49 because it is part of the documented CC20-CC54 range and appears as a reserved controller/instrument.

Runtime implementations must still enforce CC49 as reserved unless Harp support is explicitly added in a later phase.

For Phase 3B, this is documented as a validation/modeling distinction:

- JSON shape validation confirms CC49 is structurally valid.
- Plugin behavior must still treat CC49 as reserved/inactive.

## Validation Notes

PowerShell can validate that both JSON files are syntactically valid:

```powershell
Get-Content .\Schemas\orchconductor_library.schema.json -Raw | ConvertFrom-Json | Out-Null
Get-Content .\Examples\orchconductor_library_v1.example.json -Raw | ConvertFrom-Json | Out-Null
```

Full JSON Schema validation can be performed later using an external validator that supports JSON Schema Draft 2020-12.

## Phase 3B Validation Checklist

- [x] Formal JSON Schema file added
- [x] Schema syntax validated as JSON
- [x] Existing example JSON syntax remains valid
- [x] Schema documents CC20-CC54 constraints
- [x] Schema documents value range 0-127
- [x] Schema keeps CC49 structurally visible but behaviorally reserved
- [x] No C++ source changes
- [x] No MIDI behavior changes
- [x] No runtime file I/O added

## Recommended Next Phase

Phase 3C options:

1. Expand the example library to include every current hardcoded preset.
2. Add a schema validation helper script for developer-only validation.
3. Add internal C++ preset definition structs without JSON parsing.
4. Add runtime parser behind a disabled compile-time flag.

Recommended next step:

```text
Phase 3C: Complete Factory Library JSON Expansion
```

This would make the example library a full mirror of the current hardcoded preset set before runtime parsing begins.
