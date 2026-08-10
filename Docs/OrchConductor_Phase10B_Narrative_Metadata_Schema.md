# OrchConductor Phase 10B — Narrative Metadata Schema

Phase 10B extends OrchConductor's runtime preset and combi data with optional narrative/semantic metadata.

The goal of this phase is to enrich the JSON/catalog model while preserving current MIDI output behavior.

## Scope

Phase 10B is a data-model and schema-enrichment phase.

It should:

- Add optional narrative metadata to section presets and combi presets.
- Parse metadata safely from JSON when present.
- Export metadata for user combis when available.
- Preserve backward compatibility with existing JSON files.
- Preserve current preset/combi MIDI output behavior.

It should not:

- Add Narrative Scan automation.
- Add lane-selection UI.
- Add preset morphing.
- Reorder presets automatically.
- Change CC output based on metadata.

Those behaviors are reserved for later phases.

## Metadata Object

Any section preset or combi preset may include a `metadata` object:

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

The object is optional.

If the object is absent, the runtime catalog should use safe defaults.

## Numeric Metadata Fields

Numeric fields use a normalized range:

```text
0.0 = minimum / absent
1.0 = maximum / fully present
```

Recommended numeric fields:

| Field | Meaning |
|---|---|
| `energy` | Perceived musical drive or intensity |
| `density` | Orchestral thickness / number of participating players |
| `brightness` | Spectral and register brightness |
| `weight` | Low-register mass or orchestral heaviness |
| `tension` | Harmonic, timbral, or dramaturgical tension |

All numeric metadata values should be clamped to:

```text
0.0–1.0
```

## String Metadata Fields

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

## Default Metadata

If metadata is absent, the runtime representation should behave as if this object were present:

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

## Backward Compatibility Rules

Existing preset JSON files without metadata remain valid.

The parser should:

- Treat the `metadata` object as optional.
- Treat every field inside `metadata` as optional.
- Clamp numeric values to `0.0–1.0`.
- Ignore unknown metadata fields.
- Use default values for missing or invalid fields.
- Avoid failing the entire catalog load because of malformed metadata.

## Export Rules

When exporting user combis, Phase 10B should include metadata fields once the user-combi model supports them.

Initial exported metadata may use defaults:

```json
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
```

This keeps exported user combis forward-compatible with later Narrative Scan features.

## Example Utility Combi

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

## Example Narrative Combi

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

## Relationship to Later Phases

Phase 10B only prepares the data layer.

Later phases may use this metadata as follows:

| Later phase | Possible use |
|---|---|
| Phase 10C | Narrative Scan parameter traverses combis within a selected lane |
| Phase 10D | Preset Morph Mode interpolates between two preset states |
| Later form tools | OrchFormPlanner may select presets by energy/tension/density curve |

## Validation Checklist

Phase 10B is complete when:

- Runtime catalog structs include default narrative metadata.
- JSON parser accepts metadata on section presets and combis.
- Missing metadata remains valid.
- Malformed metadata does not crash loading.
- Exported user combis can include metadata.
- Existing factory fallback behavior and MIDI output are unchanged.
