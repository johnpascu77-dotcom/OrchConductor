# OrchConductor

**OrchConductor** is a JUCE-based MIDI orchestration preset sender.

It provides section and full-score orchestration controls for driving MIDI CC-based orchestral templates. The plugin is currently validated through **Phase 2B**, with **Phase 2C** serving as a stabilization and documentation checkpoint.

---

## Current Milestone

```text
Current validated milestone: Phase 2B
Current documentation/stabilization pass: Phase 2C
```

Phase 2B adds **Active Player Metadata** on top of the Phase 2A Combi Preset Engine.

Active player metadata is currently **display-only**. It does not change MIDI CC output behavior, velocity, note allocation, MIDI channels, voice routing, or articulation logic.

---

## Core Features

- Full-score MIDI CC map from **CC20 through CC54**
- Section preset dropdowns for:
  - Woodwinds
  - Brass
  - Percussion
  - Strings
- Built-in **Combi Presets** for full-orchestra and musical-use combinations
- 10 Hz UI sync timer for host/project restore behavior
- **Active Player Metadata** display:
  - per output row
  - total active players in the status line
  - visible in the MIDI map popup
- **CC49 reserved for Harp**
- Manual send controls:
  - Send Current Presets
  - Send All Off
  - optional Send on Preset Change

---

## MIDI CC Layout

The current full-score range is:

```text
CC20-CC31   Woodwinds
CC32-CC42   Brass
CC43-CC48   Percussion
CC49        Harp reserved
CC50-CC54   Strings
```

The exact section/instrument mapping is maintained in the source arrays in `OrchConductorProcessor`.

---

## Active Player Metadata

Phase 2B introduces active player count metadata.

Each visible output row now contains:

```cpp
juce::String instrumentName;
int ccNumber;
int value;
int activePlayers;
int maxPlayers;
```

The player count is derived from the current MIDI CC value:

```text
value <= 0    -> activePlayers = 0
value > 0     -> activePlayers is scaled against maxPlayers
```

The current default maximum-player profile is:

| Instrument / Area | CC | Max Players |
|---|---:|---:|
| Violin I | 50 | 8 |
| Violin II | 51 | 6 |
| Viola | 52 | 4 |
| Cello | 53 | 4 |
| Double Bass | 54 | 2 |
| Other active rows | 20-48 | 1 |
| Harp reserved | 49 | 0 / reserved |

In Phase 2B/2C this metadata is used for display only.

---

## Combi Presets

Phase 2A introduced the Combi Preset Engine.

Combi presets override the displayed and sent section values while keeping the manual section dropdowns available as fallback/manual state.

Examples include:

- `[Utility] Full Orchestra`
- `[Utility] Full Orchestra No Percussion`
- `[Utility] Chamber Orchestra`
- `[Utility] Full Strings`
- `[Utility] Full Woodwinds`
- `[Utility] Full Brass`
- `[Utility] High Orchestra`
- `[Utility] Low Orchestra`
- musical color presets such as Romantic, Cinematic, Classical, Baroque, and Solo/Feature combinations

The UI status line indicates when Combi mode is active.

---

## Validation Status

Validated through Phase 2B:

- Release build succeeds
- Plugin loads in host
- Combi Preset Engine works
- Bitwig/project restore UI synchronization works via timer refresh
- Active player totals follow Combi preset values
- MIDI Map displays values and player counts
- Send Current Presets preserves the existing MIDI CC output model
- Send All Off sends zero values
- CC49 remains reserved for Harp

---

## Build

Typical local build command:

```powershell
cmake --build build --config Release
```

---

## Git Milestones

Recent validated tags:

```text
phase-1G-validated
phase-1H-dropdowns-validated
phase-2A-combi-preset-engine
phase-2A-validated-bitwig
phase-2B-active-player-metadata
```

---

## Roadmap Direction

Likely next phases:

1. Phase 2C stabilization/docs
2. JSON library/preset externalization
3. Travel Modes
4. Optional player-count-driven MIDI behavior experiments

Potential future MIDI behavior based on player counts may include velocity scaling, channel routing, or voice allocation, but none of these are active in Phase 2B/2C.