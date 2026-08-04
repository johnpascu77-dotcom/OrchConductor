# OrchConductor Phase 3C - Complete Factory Library JSON Expansion

## Status

Phase 3C expands the Phase 3A example library into a complete JSON mirror of the current hardcoded factory preset behavior.

This remains a non-runtime documentation/data checkpoint.

## Added/Modified Files

```text
Examples/orchconductor_library_v1.example.json
Docs/OrchConductor_Phase3C_Complete_Factory_JSON.md
```

## Scope

Phase 3C updates the example library to include:

- Complete instrument map for CC20-CC54
- Reserved CC49 Harp metadata
- Active player metadata profile
- All Woodwinds section presets
- All Brass section presets
- All Percussion section presets
- All Strings section presets
- All Combi presets

## Section Preset Coverage

| Section | Factory ID Range | Count |
|---|---:|---:|
| Woodwinds | 0-19 | 20 |
| Brass | 0-16 | 17 |
| Percussion | 0-8 | 9 |
| Strings | 0-13 | 14 |

## Combi Preset Coverage

| Category | Factory ID Range |
|---|---|
| Manual | 0 |
| Utility | 1-11 |
| Romantic | 12-16 |
| Cinematic | 17-20 |
| Herrmann | 21-24 |
| Modernist | 25-26 |
| Shimmer | 27 |
| Solo | 28 |

## Important Behavior Notes

Phase 3C does not change plugin behavior.

The JSON file is still not parsed by the plugin.

The current C++ hardcoded preset logic remains authoritative at runtime.

## Partial Values

The expanded factory JSON preserves current partial-value behavior:

| Preset | CC | Value |
|---|---:|---:|
| Strings / Low Strings | 52 | 64 |
| Solo / English Horn Lament | 54 | 64 |

All other active values in the current factory expansion are `127`.

## Reserved CC49

CC49 remains structurally documented as Harp/reserved.

No factory preset in the expanded JSON assigns an active value to CC49.

Future runtime loading should continue to force or clamp CC49 to zero unless Harp support is explicitly implemented.

## Validation

JSON syntax can be checked with:

```powershell
Get-Content .\Examples\orchconductor_library_v1.example.json -Raw | ConvertFrom-Json | Out-Null
Get-Content .\Schemas\orchconductor_library.schema.json -Raw | ConvertFrom-Json | Out-Null
```

Full JSON Schema validation can be performed later using a Draft 2020-12 compatible validator.

## Non-Goals

Phase 3C does not:

- Add runtime JSON parsing
- Add file I/O
- Add a library browser
- Modify C++ source
- Modify MIDI output behavior
- Replace hardcoded presets
- Modify active player calculation

## Phase 3C Validation Checklist

- [x] Complete Woodwinds factory preset expansion
- [x] Complete Brass factory preset expansion
- [x] Complete Percussion factory preset expansion
- [x] Complete Strings factory preset expansion
- [x] Complete Combi factory preset expansion
- [x] CC49 remains reserved/inactive
- [x] Partial values preserved
- [x] JSON remains schema-versioned
- [x] No C++ source changes
- [x] No runtime behavior changes

## Recommended Next Phase

Recommended next step:

```text
Phase 3D: Developer JSON Validation Script
```

Phase 3D can add a developer-only validation script that checks:

- JSON syntax
- Required files exist
- No preset activates CC49
- CC values remain in CC20-CC54
- Values remain in 0-127
- Factory preset counts match expectations

This should still avoid runtime plugin changes.
