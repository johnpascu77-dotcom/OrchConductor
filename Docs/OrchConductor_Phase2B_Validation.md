# OrchConductor Phase 2B Validation

## Phase

```text
Phase 2B: Active Player Metadata
```

## Status

```text
Status: Complete
Build: Passing
Plugin UI: Validated
Git tag: phase-2B-active-player-metadata
Commit: 85fed06 Add Phase 2B active player metadata
```

---

## Objective

Phase 2B adds an active-player metadata layer to the existing OrchConductor MIDI CC preset system.

The purpose is to calculate and display the number of active players represented by the current orchestration preset, without changing MIDI output behavior.

---

## Source Changes

Phase 2B extends `OutputRow` with:

```cpp
int activePlayers;
int maxPlayers;
```

The output rows now represent:

```cpp
juce::String instrumentName;
int ccNumber;
int value;
int activePlayers;
int maxPlayers;
```

The UI reads these values for:

- status-line total active players
- visible output table
- MIDI map popup

---

## Default Player Profile

| Area | CC | Max Players |
|---|---:|---:|
| Violin I | 50 | 8 |
| Violin II | 51 | 6 |
| Viola | 52 | 4 |
| Cello | 53 | 4 |
| Double Bass | 54 | 2 |
| Woodwinds | 20-31 | 1 each |
| Brass | 32-42 | 1 each |
| Percussion | 43-48 | 1 each |
| Harp | 49 | reserved |

---

## Calculation Rule

For each row:

```text
value <= 0    -> activePlayers = 0
value > 0     -> activePlayers = scaled value against maxPlayers
```

The scaled result is clamped between:

```text
1 and maxPlayers
```

for active non-zero values.

---

## Important Constraint

Phase 2B metadata is **display-only**.

It does not alter:

- MIDI CC numbers
- MIDI CC values
- velocity
- MIDI channel
- note allocation
- voice allocation
- articulation routing
- host automation behavior

---

## Validation Performed

### Build

Command:

```powershell
cmake --build build --config Release
```

Result:

```text
Build passed
```

### Plugin UI

Confirmed visible:

```text
Build: Phase 2B
Phase 2B: Active player metadata enabled | Future: JSON library / travel modes
Phase 2B: Full-score CC20-CC54 | Combi Presets | Active Players | CC49 reserved for Harp
```

### Active Player Count

Confirmed active player count follows Combi preset values.

Example validated:

```text
Combi active: [Utility] High Orchestra | Active players: 26 | Harp reserved | Auto-send: Off
```

Expected string-only profile:

```text
[Utility] Full Strings -> Active players: 24
```

Breakdown:

| Instrument | Max Players |
|---|---:|
| Violin I | 8 |
| Violin II | 6 |
| Viola | 4 |
| Cello | 4 |
| Double Bass | 2 |
| Total | 24 |

---

## Phase 2B Fixes During Validation

Initial implementation compiled after repairing fallback row initializers.

A second validation issue was found:

```text
Active players displayed as 0 when a Combi preset was active
```

Cause:

```text
Display row functions were still using manual section preset values instead of Combi preset values.
```

Fix:

```text
getOutputRow
getWoodwindsOutputRow
getBrassOutputRow
getPercussionOutputRow
```

were updated to use:

```cpp
isCombiModeActive()
    ? getCombiPresetValueForCc (cc)
    : getSectionPresetValueForIndex (index)
```

Result:

```text
Active player totals now follow Combi mode correctly.
```

---

## Final Phase 2B Result

```text
Phase 2B: Complete
Build: Passing
Plugin UI: Validated
Active players: Working
MIDI behavior: Unchanged
Git: Committed and tagged
```