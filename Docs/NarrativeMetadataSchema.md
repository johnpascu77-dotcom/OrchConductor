# Phase 10B — Narrative Metadata Schema

OrchConductor Phase 10B extends preset and combi data with optional narrative/semantic metadata.

The goal of this phase is to enrich the JSON/catalog model without changing current MIDI output behavior.

## Design goals

- Preserve backward compatibility with existing factory and user combis.
- Keep all new metadata fields optional.
- Use safe defaults when metadata is absent.
- Support later Narrative Scan and preset morphing features.
- Avoid changing current preset sending behavior in Phase 10B.

## Metadata object

Presets and combis may include a `metadata` object:

```json
{
  "metadata": {
    "energy": 0.0,
    "density": 0.0,
    "brightness": 0.0,
    "weight": 0.0,
    "tension": 0.0,
    "register": "mixed",
    "role": "utility",
    "transition_behavior": "neutral",
    "narrative_lane": ""
  }
}
```

## Numeric fields

Numeric fields use a normalized range:

```text
0.0 = minimum / absent
1.0 = maximum / fully present
```

Recommended numeric fields:

| Field | Meaning |
|---|---|
| `energy` | Perceived musical drive or intensity |
| `density` | Number of active players / thickness |
| `brightness` | Spectral/register brightness |
| `weight` | Low-register mass or orchestral heaviness |
| `tension` | Harmonic/timbral dramatic tension |

All numeric values should be clamped to:

```text
0.0–1.0
```

## String fields

### `register`

Suggested values:

```text
low
mid
high
mixed
full
```

Default:

```text
mixed
```

### `role`

Suggested values:

```text
utility
pad
melody
countermelody
rhythm
accent
texture
transition
climax
```

Default:

```text
utility
```

### `transition_behavior`

Suggested values:

```text
neutral
smooth
sudden
build
release
sustain
```

Default:

```text
neutral
```

### `narrative_lane`

Optional free-form lane identifier for later Narrative Scan support.

Examples:

```text
Organic Build
Tension Rise
Climax
Release
```

Default:

```text

```

## Backward compatibility

The `metadata` object is optional.

If absent, the runtime catalog should use these defaults:

```json
{
  "energy": 0.0,
  "density": 0.0,
  "brightness": 0.0,
  "weight": 0.0,
  "tension": 0.0,
  "register": "mixed",
  "role": "utility",
  "transition_behavior": "neutral",
  "narrative_lane": ""
}
```

Existing JSON files without metadata remain valid.

## Phase 10B non-goals

Phase 10B does not implement:

- Narrative Scan automation
- Lane selection UI
- Preset morphing
- MIDI output changes based on metadata
- Automatic preset ordering by metadata

These are reserved for later phases.

## Example combi preset

```json
{
  "id": 6,
  "name": "[Utility] Full Woodwinds",
  "sections": {
    "woodwinds": 19,
    "brass": 0,
    "percussion": 0,
    "strings": 0
  },
  "metadata": {
    "energy": 0.35,
    "density": 0.45,
    "brightness": 0.65,
    "weight": 0.15,
    "tension": 0.1,
    "register": "mixed",
    "role": "utility",
    "transition_behavior": "neutral",
    "narrative_lane": ""
  }
}
```

## Example narrative combi preset

```json
{
  "id": 101,
  "name": "[Narrative] Organic Build 1",
  "sections": {
    "woodwinds": 4,
    "brass": 1,
    "percussion": 0,
    "strings": 6
  },
  "metadata": {
    "energy": 0.25,
    "density": 0.3,
    "brightness": 0.4,
    "weight": 0.25,
    "tension": 0.35,
    "register": "mixed",
    "role": "texture",
    "transition_behavior": "build",
    "narrative_lane": "Organic Build"
  }
}
```
