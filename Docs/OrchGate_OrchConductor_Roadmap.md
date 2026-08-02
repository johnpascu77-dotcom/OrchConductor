# OrchGate / OrchConductor Roadmap

## Purpose

This document captures the future concept for a companion orchestration system to **OrchNoteMapper**.

The current plugin, **OrchNoteMapper**, solves the problem:

> If a MIDI track receives notes, how do we map them into a playable and instrument-appropriate orchestral range, while also handling keyswitch remapping safely?

The proposed companion system solves the next musical problem:

> When should each orchestral instrument participate?

Together, these two ideas form a larger orchestration workflow:

```text
Continuous or generative MIDI material
        ↓
OrchNoteMapper
        ↓
OrchGate / OrchConductor participation control
        ↓
Orchestral instrument
```

The goal is to make a full orchestral template behave like a living orchestration system, where every track can receive related musical material, but only selected instruments are allowed to play at any given time.

This creates “holes” in a continuous full-score texture and turns a dense generative source into a shaped, orchestrated output.

---

## Core Concept

A full orchestral template may contain one instance of OrchNoteMapper on every track, each with the corresponding instrument preset.

Example:

```text
Piccolo          OrchNoteMapper preset: Piccolo
Flute            OrchNoteMapper preset: Flute
Oboe             OrchNoteMapper preset: Oboe
English Horn     OrchNoteMapper preset: English Horn
Clarinet         OrchNoteMapper preset: Clarinet
Bass Clarinet    OrchNoteMapper preset: Bass Clarinet
Bassoon          OrchNoteMapper preset: Bassoon
Contrabassoon    OrchNoteMapper preset: Contrabassoon

French Horn      OrchNoteMapper preset: French Horn
Trumpet          OrchNoteMapper preset: Trumpet
Trombone         OrchNoteMapper preset: Trombone
Bass Trombone    OrchNoteMapper preset: Bass Trombone
Tuba             OrchNoteMapper preset: Tuba

Timpani          OrchNoteMapper preset: Timpani

Violin           OrchNoteMapper preset: Violin
Viola            OrchNoteMapper preset: Viola
Cello            OrchNoteMapper preset: Cello
Double Bass      OrchNoteMapper preset: Double Bass

Glockenspiel     OrchNoteMapper preset: Glockenspiel
Xylophone        OrchNoteMapper preset: Xylophone
Marimba          OrchNoteMapper preset: Marimba
Vibraphone       OrchNoteMapper preset: Vibraphone
Tubular Bells    OrchNoteMapper preset: Tubular Bells
```

Each track receives musical input, possibly from:

- a MIDI clip
- a generative MIDI source
- DAW modulators
- randomized note streams
- shared harmonic material
- copied or derived MIDI clips
- algorithmic composition tools

OrchNoteMapper makes the material playable for each instrument.

The future companion plugin controls whether each track currently speaks.

The resulting orchestration is created by selective participation:

```text
Full musical material exists everywhere.
The orchestration is created by deciding who is allowed to sound.
```

---

## Proposed Modules

### OrchGate

**OrchGate** is the first practical companion plugin.

It is a per-track MIDI gate / orchestration mask.

Recommended chain:

```text
MIDI Source / Randomizer
        ↓
OrchNoteMapper
        ↓
OrchGate
        ↓
Instrument
```

OrchGate decides whether MIDI output reaches the instrument.

Basic behavior:

```text
If gate is open:
    pass MIDI notes normally

If gate is closed:
    suppress musical note-ons
    safely handle note-offs
    optionally pass keyswitches
    optionally pass CCs
```

The key idea is that the instrument can be temporarily muted at the MIDI level without disabling the track or plugin chain.

### OrchConductor

**OrchConductor** is the later, more advanced global-control idea.

It would act as a conductor/control layer for multiple OrchGate instances.

Possible architecture:

```text
                OrchConductor / Conductor Track
                     ↓
            MIDI CCs / Notes / Automation
                     ↓
---------------------------------------------------
Piccolo Track       OrchNoteMapper → OrchGate → Instrument
Flute Track         OrchNoteMapper → OrchGate → Instrument
Oboe Track          OrchNoteMapper → OrchGate → Instrument
Clarinet Track      OrchNoteMapper → OrchGate → Instrument
Bassoon Track       OrchNoteMapper → OrchGate → Instrument
Horn Track          OrchNoteMapper → OrchGate → Instrument
Trumpet Track       OrchNoteMapper → OrchGate → Instrument
Trombone Track      OrchNoteMapper → OrchGate → Instrument
Tuba Track          OrchNoteMapper → OrchGate → Instrument
Violin Track        OrchNoteMapper → OrchGate → Instrument
Viola Track         OrchNoteMapper → OrchGate → Instrument
Cello Track         OrchNoteMapper → OrchGate → Instrument
Bass Track          OrchNoteMapper → OrchGate → Instrument
---------------------------------------------------
```

OrchConductor would control section density, register focus, call-and-response, tutti amount, and other orchestration-level decisions.

This should come later. The first useful step is OrchGate.

---

## Recommended Development Strategy

The recommended development path is layered.

Do not start with complex cross-instance communication.

Start with a robust per-track gate, then make it smarter.

```text
1. Build simple per-track OrchGate.
2. Add keyswitch-aware muting.
3. Add better mute modes.
4. Add probability/pattern participation.
5. Add conductor-track MIDI CC control.
6. Eventually consider true OrchConductor master/slave architecture.
```

---

## Phase A — Simple Per-Track MIDI Gate

### Goal

Build a small companion MIDI plugin:

```text
OrchGate
```

It should be simple, stable, automatable, and host-friendly.

### Initial Parameters

```text
Gate Open: On / Off
Mute Notes: On / Off
Pass CC While Muted: On / Off
Pass Pitch Bend While Muted: On / Off
Pass Aftertouch While Muted: On / Off
Send Note-Offs On Close: On / Off
Mute Mode: Hard Gate / No New Notes
```

### Core Behaviors

When open, pass MIDI through normally.

When closed:

```text
normal note-ons  → blocked
note-offs        → passed or consumed safely
CCs              → optionally passed
keyswitches      → optionally passed
```

When the gate changes from open to closed:

```text
send note-offs for all currently active notes
clear active-note tracking
```

This prevents stuck notes.

### Technical Requirement: Active Note Tracking

OrchGate must track active notes:

```cpp
bool activeNotes[16][128];
```

When a note-on passes through:

```text
activeNotes[channel][note] = true
```

When a note-off passes through:

```text
activeNotes[channel][note] = false
```

When the gate closes:

```text
for every active note:
    emit note-off
    mark inactive
```

This is essential.

---

## Phase B — Keyswitch-Aware Gate

### Goal

Make OrchGate aware of keyswitch ranges so that articulations can still be prepared while musical notes are muted.

### Problem

If the gate blocks all notes while muted, then keyswitches are blocked too.

That means when the instrument opens again, it may still be on an old articulation.

### Solution

Add keyswitch pass-through logic.

### Parameters

```text
Keyswitch Mode: Off / Range
KS Min: 24
KS Max: 35
Pass Keyswitches While Muted: On / Off
```

Recommended chain:

```text
MIDI Source
        ↓
OrchNoteMapper
        ↓
OrchGate
        ↓
Instrument
```

This allows OrchNoteMapper to normalize keyswitches first.

Then OrchGate only needs to know the final destination keyswitch range, for example:

```text
24 - 35
```

### Musical Benefit

Example:

```text
Bar 1:
    Violin muted
    keyswitch changes to tremolo still pass through

Bar 2:
    Violin gate opens
    first sounding note is already tremolo
```

---

## Phase C — Better Mute Modes

### Goal

Avoid making the gate feel like a crude on/off switch.

### Proposed Mute Modes

```text
Hard Gate
No New Notes
Expression Fade
```

### Hard Gate

When the gate closes:

```text
immediately send note-offs for active notes
block new notes
```

Best for rhythmic cuts and precise holes.

### No New Notes

When the gate closes:

```text
do not send immediate note-offs
allow already-sounding notes to finish
block only new note-ons
```

Best for natural phrase endings.

### Expression Fade

When the gate closes:

```text
fade CC11 or CC7 toward 0
optionally send note-offs after fade time
```

Best for soft orchestral fades and long textures.

---

## Phase D — Pattern / Probability Gate

### Goal

Let OrchGate generate participation patterns automatically.

### Parameters

```text
Gate Mode:
    Manual
    Periodic
    Probability
    Phrase Holes
    Random Walk
    Call/Response

Grid:
    1/16
    1/8
    1/4
    1/2
    1 bar
    2 bars
    4 bars

Density:
    0 - 100%

Seed:
    integer

Minimum On Duration:
    musical time value

Minimum Off Duration:
    musical time value

Pattern Offset:
    musical time value
```

---

## Phase E — DAW Automation Workflow

### Goal

Make OrchGate immediately useful without complex global communication.

Each track exposes:

```text
Gate Open
Density
Mute Mode
Pass KS While Muted
```

Then the DAW can automate each track.

### Pros

- Simple
- Reliable
- Host-friendly
- No cross-instance communication
- Easy to edit visually
- Easy to record automation
- Works in most DAWs

---

## Phase F — Conductor Track via MIDI CC

### Goal

Control many OrchGate instances from one central MIDI/automation track.

Instead of true plugin-to-plugin communication, use ordinary MIDI CCs.

### Example CC Map

```text
CC20 = Strings Activity
CC21 = Woodwinds Activity
CC22 = Brass Activity
CC23 = Percussion Activity

CC30 = High Register Activity
CC31 = Middle Register Activity
CC32 = Low Register Activity

CC40 = Tutti Amount
CC41 = Sparse / Dense Balance
CC42 = Call / Response Amount
CC43 = Soloist Focus
```

Each OrchGate instance has profile settings:

```text
Section: Strings / Woodwinds / Brass / Percussion / Custom
Register: High / Mid / Low
Role: Melody / Harmony / Bass / Punctuation / Texture / Color
Weight: 0 - 100%
Threshold: 0 - 100%
```

---

## Phase G — True Master/Slave OrchConductor

### Goal

Create a dedicated master plugin that communicates with all track-level gate instances.

This could show an orchestration matrix and manage participation globally.

### Recommendation

Do not start here.

Build OrchGate first. Then build MIDI CC conductor workflows. Only later consider true master/slave communication.

---

## Musical Intelligence Ideas

Future participation rules may include:

- Section density
- Register-aware participation
- Call and response
- Anti-overcrowding
- Role-based behavior
- Phrase-level logic

Example roles:

```text
Melody
Countermelody
Harmony
Bass
Punctuation
Texture
Color
Percussion Accent
```

---

## Technical Priorities

Highest priority:

```text
Prevent stuck notes.
```

Requirements:

- track active notes per channel/note
- send note-offs when gate closes
- clean state on transport stop
- clean state on plugin reset
- handle all-notes-off messages
- handle sustain pedal carefully

Other priorities:

- host automation compatibility
- MIDI safety
- sample-accurate MIDI buffer handling
- transport awareness for later pattern modes

---

## Suggested First Version of OrchGate

### Name

```text
OrchGate
```

### Subtitle

```text
MIDI Orchestration Mask
```

### Minimal UI

```text
Gate Open:                On / Off
Mute Mode:                Hard Gate / No New Notes
Pass Keyswitches:         On / Off
KS Range:                 24 - 35
Pass CC While Muted:      On / Off
Send Note-Offs On Close:  On / Off

Status:
    Gate: Open / Closed
    Active Notes: N
    Last Passed Note: X
    Last Blocked Note: Y
    Last Passed KS: Z
```

---

## Longer-Term Vision

The complete system could eventually become:

```text
OrchNoteMapper
    Maps incoming material to playable instrument ranges.
    Handles keyswitch remapping.
    Protects low/high keyswitch zones.
    Makes random/generative MIDI musically usable.

OrchGate
    Decides whether each instrument speaks.
    Creates orchestration holes.
    Handles MIDI muting safely.
    Supports keyswitch-aware muting.

OrchConductor
    Controls many OrchGate instances.
    Shapes orchestral density, sections, roles, and phrases.
    Turns continuous MIDI material into orchestrated output.
```

Together:

```text
Range mapping + keyswitch intelligence + participation control
=
a playable generative orchestration environment
```

---

## Current Priority

Before starting OrchGate, finish and polish the current OrchNoteMapper roadmap.

Current plugin state:

- Instrument preset mapping works.
- Keyswitch protection and remapping work.
- Generic keyswitch destination presets work.
- Randomized notes and randomized keyswitches produce musical results.
- Phase 7C preset reordering and Bass Clarinet addition are complete.
- The system is already musically useful.

Next immediate work should remain focused on finishing and polishing OrchNoteMapper before starting the companion plugin.
