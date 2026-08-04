# OrchConductor JSON Library Schema v1

## Purpose

This document defines the proposed v1 JSON library format for OrchConductor preset externalization.

The schema is currently documentation-only. OrchConductor does not yet parse this format at runtime.

## Top-Level Object

```json
{
  "schema": "orchconductor.library",
  "schemaVersion": 1,
  "libraryName": "Factory Library",
  "libraryVersion": "1.0.0",
  "pluginTarget": "OrchConductor",
  "midi": {},
  "instruments": [],
  "playerProfile": {},
  "sectionPresets": {},
  "combiPresets": []
}
```

## Required Fields

| Field | Type | Description |
|---|---|---|
| `schema` | string | Must be `orchconductor.library`. |
| `schemaVersion` | integer | Schema version. Current proposed version is `1`. |
| `libraryName` | string | Human-readable library name. |
| `libraryVersion` | string | Library content version. |
| `pluginTarget` | string | Intended plugin/application target. |
| `midi` | object | MIDI CC range and reserved-controller metadata. |
| `instruments` | array | Instrument rows and CC assignments. |
| `playerProfile` | object | Active player metadata profile. |
| `sectionPresets` | object | Section preset definitions grouped by section. |
| `combiPresets` | array | Cross-section preset definitions. |

## MIDI Object

```json
{
  "channel": 1,
  "ccRange": {
    "min": 20,
    "max": 54
  },
  "reservedControllers": [
    {
      "cc": 49,
      "name": "Harp",
      "status": "reserved",
      "defaultValue": 0
    }
  ]
}
```

### MIDI Field Rules

| Field | Rule |
|---|---|
| `channel` | MIDI channel used by current plugin output. Current behavior sends channel 1. |
| `ccRange.min` | Lowest supported CC. Current value: 20. |
| `ccRange.max` | Highest supported CC. Current value: 54. |
| `reservedControllers` | Documents reserved CCs. CC49/Harp is reserved in v1. |

## Instrument Object

Each instrument row should use:

```json
{
  "id": "woodwinds.piccolo",
  "name": "Piccolo",
  "section": "woodwinds",
  "cc": 20,
  "reserved": false
}
```

### Instrument Fields

| Field | Type | Description |
|---|---|---|
| `id` | string | Stable machine-readable ID. |
| `name` | string | UI/display name. |
| `section` | string | One of `woodwinds`, `brass`, `percussion`, `reserved`, `strings`. |
| `cc` | integer | MIDI CC number. |
| `reserved` | boolean | Whether this row is reserved/inactive. |

## Player Profile

```json
{
  "defaultMaxPlayers": 1,
  "reservedMaxPlayers": 0,
  "overrides": [
    { "cc": 50, "maxPlayers": 8 },
    { "cc": 51, "maxPlayers": 6 },
    { "cc": 52, "maxPlayers": 4 },
    { "cc": 53, "maxPlayers": 4 },
    { "cc": 54, "maxPlayers": 2 }
  ]
}
```

### Player Profile Rules

- Active player metadata is display-only.
- Values must not alter MIDI output.
- Runtime behavior should keep CC output and active-player metadata separate.
- Rows without overrides use `defaultMaxPlayers`.
- Reserved rows use `reservedMaxPlayers`.

## Section Presets

Section presets are grouped by section:

```json
{
  "woodwinds": [],
  "brass": [],
  "percussion": [],
  "strings": []
}
```

Each section preset uses:

```json
{
  "id": "woodwinds.full_woodwinds",
  "name": "Full Woodwinds",
  "section": "woodwinds",
  "factoryId": 19,
  "values": [
    { "cc": 20, "value": 127 },
    { "cc": 21, "value": 127 }
  ]
}
```

### Section Preset Fields

| Field | Type | Description |
|---|---|---|
| `id` | string | Stable machine-readable preset ID. |
| `name` | string | UI/display preset name. |
| `section` | string | Section group. |
| `factoryId` | integer/null | Current internal hardcoded ID, if applicable. |
| `values` | array | Explicit CC/value list. |

## Combi Presets

Combi presets use:

```json
{
  "id": "utility.full_orchestra",
  "name": "[Utility] Full Orchestra",
  "category": "Utility",
  "factoryId": 2,
  "values": [
    { "cc": 20, "value": 127 },
    { "cc": 21, "value": 127 }
  ]
}
```

### Combi Preset Fields

| Field | Type | Description |
|---|---|---|
| `id` | string | Stable machine-readable preset ID. |
| `name` | string | UI/display preset name. |
| `category` | string | UI/category label. |
| `factoryId` | integer/null | Current internal hardcoded ID, if applicable. |
| `description` | string | Optional description. |
| `values` | array | Explicit CC/value list. |

## Value Object

```json
{
  "cc": 50,
  "value": 127
}
```

### Value Rules

| Field | Rule |
|---|---|
| `cc` | Integer in supported CC range. |
| `value` | Integer from 0 through 127. |

Recommended validation:

- Reject non-integer CCs.
- Reject CCs outside CC20-CC54.
- Clamp or reject values outside 0-127.
- Ignore or force CC49 to 0 unless Harp support is enabled in a later phase.

## Factory ID Notes

`factoryId` is included only as a migration bridge from the current hardcoded enum/int preset system.

Future external libraries should not rely on `factoryId` for identity.

Stable string `id` values should become the durable identity.

## Compatibility Notes

Schema v1 is designed to preserve current Phase 2C behavior.

A future runtime loader should be able to read v1 libraries without requiring changes to the existing MIDI CC map.
