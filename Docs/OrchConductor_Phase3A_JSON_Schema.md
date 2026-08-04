# OrchConductor Phase 3A - JSON Schema Design

## Status

Phase 3A is a design-only checkpoint for future preset/library externalization.

This phase documents the proposed JSON library structure for OrchConductor but does not add runtime parsing or change MIDI behavior.

## Scope

Phase 3A defines the proposed JSON model for:

- Instrument map metadata
- MIDI CC assignments
- Reserved controllers
- Active player metadata profile
- Section presets
- Combi presets
- Factory library examples
- Schema versioning

## Explicit Non-Goals

Phase 3A does not:

- Add JSON parsing to the plugin
- Add file browser or library loading UI
- Replace current hardcoded presets
- Change generated MIDI CC output
- Change active player calculation
- Change CC49/Harp behavior
- Add velocity scaling or routing behavior based on active players

## Current Internal MIDI Map

OrchConductor currently uses MIDI CC20 through CC54.

### Woodwinds

| Instrument | CC |
|---|---:|
| Piccolo | 20 |
| Flute 1 | 21 |
| Flute 2 | 22 |
| Oboe 1 | 23 |
| Oboe 2 | 24 |
| English Horn | 25 |
| Clarinet 1 | 26 |
| Clarinet 2 | 27 |
| Bass Clarinet | 28 |
| Bassoon 1 | 29 |
| Bassoon 2 | 30 |
| Contrabassoon | 31 |

### Brass

| Instrument | CC |
|---|---:|
| Horn 1 | 32 |
| Horn 2 | 33 |
| Horn 3 | 34 |
| Horn 4 | 35 |
| Trumpet 1 | 36 |
| Trumpet 2 | 37 |
| Trumpet 3 | 38 |
| Trombone 1 | 39 |
| Trombone 2 | 40 |
| Bass Trombone | 41 |
| Tuba | 42 |

### Percussion

| Instrument | CC |
|---|---:|
| Timpani | 43 |
| Glockenspiel | 44 |
| Xylophone | 45 |
| Marimba | 46 |
| Vibraphone | 47 |
| Tubular Bells | 48 |

### Reserved

| Instrument | CC | Status |
|---|---:|---|
| Harp | 49 | Reserved |

CC49 is reserved for Harp and currently remains inactive in the plugin behavior.

### Strings

| Instrument | CC |
|---|---:|
| Violin I | 50 |
| Violin II | 51 |
| Viola | 52 |
| Cello | 53 |
| Double Bass | 54 |

## Active Player Metadata Profile

Active player metadata is currently display-only.

It is derived from the MIDI CC value and the maximum player count for each row.

Current maximum player counts:

| Instrument/Row | CC | Max Players |
|---|---:|---:|
| Violin I | 50 | 8 |
| Violin II | 51 | 6 |
| Viola | 52 | 4 |
| Cello | 53 | 4 |
| Double Bass | 54 | 2 |
| All other active rows | CC20-48 | 1 |
| Harp | 49 | 0/reserved |

The intended formula remains:

```text
activePlayers = round((ccValue / 127.0) * maxPlayers)
```

with inactive/reserved rows resolving to zero active players.

## Proposed Library JSON

The proposed library document is represented as:

```json
{
  "schema": "orchconductor.library",
  "schemaVersion": 1,
  "libraryName": "Factory Library",
  "pluginTarget": "OrchConductor",
  "midi": {},
  "instruments": [],
  "playerProfile": {},
  "sectionPresets": {},
  "combiPresets": []
}
```

## Preset Value Rules

Preset values are stored explicitly as CC/value pairs.

A value of `0` means inactive.

A value of `127` means fully active.

Intermediate values are allowed. The current factory combi set uses one partial value:

```text
[Solo] English Horn Lament:
CC54 Double Bass = 64
```

## Reserved Controller Rule

JSON libraries may include reserved controllers for documentation, but Phase 3A recommends that runtime implementations always enforce hardcoded reserved-controller safety.

For v1:

```json
{
  "cc": 49,
  "name": "Harp",
  "status": "reserved",
  "defaultValue": 0
}
```

Future runtime loaders should ignore or clamp any preset value targeting CC49 unless Harp support is explicitly implemented in a later phase.

## Phase 3A Validation Checklist

- [x] JSON schema direction documented
- [x] Current CC map documented
- [x] Reserved CC49 behavior documented
- [x] Active player profile documented
- [x] Example library file added
- [x] No runtime behavior changed
- [x] No MIDI output behavior changed
- [x] No hardcoded preset behavior changed

## Recommended Next Phase

Phase 3B should be a non-runtime C++ preparation phase or a formal schema-validation phase.

Possible Phase 3B options:

1. Add a JSON schema file under `Schemas/`.
2. Add internal preset data structs without loading external files.
3. Add tests/helpers for validating CC/value ranges.
4. Add a disabled experimental parser behind a compile-time flag.

Runtime user-facing library loading should remain out of scope until the schema and migration model are stable.
