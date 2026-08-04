# OrchGate / OrchConductor / OrchNoteMapper Updated Roadmap

Date: 2026-08-03  
Status: Updated after OrchConductor Phase 1H validation

---

## Purpose

This document updates the original OrchGate / OrchConductor roadmap after the successful validation of the current three-plugin orchestration workflow:

```text
OrchNoteMapper
    maps incoming MIDI material into playable instrument ranges
    handles keyswitch protection/remapping

OrchGate
    acts as a per-track MIDI participation gate
    decides whether each instrument track is allowed to speak

OrchConductor
    acts as a global orchestration controller
    sends MIDI CC participation states to many OrchGate instances
```

The original roadmap correctly identified the long-term goal:

```text
Range mapping
+
keyswitch intelligence
+
participation control
=
a playable generative orchestration environment
```

The current implementation has now reached that goal in practical form inside Bitwig.

This updated roadmap preserves the original vision but revises the architecture around the validated design:

```text
Full-score individual-instrument CC control
rather than abstract section-level conductor CCs.
```

---

# 1. Current Concept

The core workflow is:

```text
MIDI clip / generative source / Bitwig randomization
        ↓
OrchNoteMapper
        ↓
OrchGate
        ↓
Instrument
```

Global participation is controlled by:

```text
OrchConductor
        ↓
MIDI CC20–CC54
        ↓
OrchGate instances on orchestral tracks
```

The important musical principle remains:

```text
Full musical material may exist everywhere.
The orchestration is created by deciding who is allowed to sound.
```

This creates controlled holes, layers, doublings, textures, and orchestral combinations from shared or related MIDI material.

---

# 2. Validated Current Architecture

## 2.1 OrchNoteMapper

Current role:

```text
Make incoming MIDI playable for a target instrument.
Protect/remap keyswitches.
Normalize random or generative note material into instrumental ranges.
```

Current practical status:

```text
Functional and musically useful.
Bitwig device presets can be used for customized per-track mapper identities.
No urgent changes required.
```

Typical use:

```text
Same or related MIDI clips can be pasted across many tracks.
Each track has its own OrchNoteMapper preset.
Per-track Bitwig Randomize devices can create variation.
OrchNoteMapper keeps the result within playable ranges.
```

---

## 2.2 OrchGate

Current role:

```text
Per-track MIDI participation gate.
Receives local MIDI material.
Allows or suppresses musical note output depending on gate state.
Can be controlled manually or by MIDI CC.
```

Current validated features include:

```text
Manual Gate
CC Gate Enable
CC Number
CC Threshold
Invert CC
Participation
Mute Mode
Pass Keyswitches
KS Min
KS Max
```

OrchGate remains intentionally simple:

```text
It does not know orchestration.
It only knows whether this track is currently allowed to pass notes.
```

This makes it robust and reusable.

---

## 2.3 OrchConductor

Current role:

```text
Global participation controller.
Sends full-score MIDI CC values to many OrchGate instances.
```

Current validated behavior:

```text
Full-score individual instrument CC map.
Expanded individual-chair dropdown presets.
Manual section combinations.
Send Current Presets.
Send All Off.
Send on Preset Change.
MIDI Map popup.
```

Current musical result:

```text
The user can freely flip through section dropdowns and create orchestration combinations manually.
OrchGate instances respond correctly.
Shared MIDI material becomes orchestrated by participation control.
```

---

# 3. Validated Full-Score MIDI CC Contract

The central design pivot is the full-score CC registry:

```text
Each orchestral template track receives one stable CC number.
Each OrchGate listens to the CC number assigned to its track.
OrchConductor sends CC values for all mapped instruments.
```

Current map:

```text
Woodwinds

CC20  Piccolo
CC21  Flute 1
CC22  Flute 2
CC23  Oboe 1
CC24  Oboe 2
CC25  English Horn
CC26  Clarinet 1
CC27  Clarinet 2
CC28  Bass Clarinet
CC29  Bassoon 1
CC30  Bassoon 2
CC31  Contrabassoon

Brass

CC32  Horn 1
CC33  Horn 2
CC34  Horn 3
CC35  Horn 4
CC36  Trumpet 1
CC37  Trumpet 2
CC38  Trumpet 3
CC39  Trombone 1
CC40  Trombone 2
CC41  Bass Trombone
CC42  Tuba

Melodic Percussion

CC43  Timpani
CC44  Glockenspiel
CC45  Xylophone
CC46  Marimba
CC47  Vibraphone
CC48  Tubular Bells

Reserved

CC49  Harp

Strings

CC50  Violin I
CC51  Violin II
CC52  Viola
CC53  Cello
CC54  Double Bass
```

This CC contract is now the backbone of the system.

---

# 4. Architectural Pivot From the Original Roadmap

The original roadmap imagined possible high-level conductor CCs such as:

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

This idea remains musically valuable, but it should not be the low-level communication contract.

The validated architecture is better:

```text
Low-level contract:
    individual instrument CCs

Higher-level musical behavior:
    generated inside OrchConductor as presets, masks, density logic, or travel modes
```

Therefore:

```text
OrchGate should remain a simple per-track gate.
OrchConductor should become the intelligent orchestration-mask engine.
```

---

# 5. Completed Milestones

## Phase 0 — OrchNoteMapper Foundation

Status:

```text
Completed before OrchGate/OrchConductor work.
```

Validated concepts:

```text
Instrument preset mapping works.
Keyswitch protection and remapping work.
Generic keyswitch destination presets work.
Randomized notes and randomized keyswitches produce musical results.
The plugin is musically useful in Bitwig.
```

---

## Phase 1A–1F — OrchGate Foundation

Status:

```text
Completed and validated.
```

Validated concepts:

```text
Per-track MIDI gate.
Safe muting.
Manual gate.
CC gate.
Threshold behavior.
Invert CC.
Pass keyswitches while muted.
No New Notes mode.
Useful status feedback.
```

Important design conclusion:

```text
OrchGate should stay instrument-agnostic.
```

---

## Phase 1G — Full-Score MIDI Map

Status:

```text
Completed, validated, and tagged.
```

Summary:

```text
OrchConductor switched from section-style CC blocks to a full-score individual instrument registry.
CC20–CC54 became the stable template-aligned participation map.
CC49 was reserved for Harp.
The main UI was simplified.
The full map was moved to a Show MIDI Map popup.
```

Result:

```text
OrchConductor, OrchGate, and the Bitwig orchestral template now share a stable MIDI contract.
```

---

## Phase 1H — Expanded Full-Score Dropdown Presets

Status:

```text
Completed, validated, committed, and tagged.
```

Summary:

```text
Section dropdowns were expanded to match the full-score CC map.
Individual chairs can be selected directly.
Auto-send behavior was fixed for all sections.
Section-specific preset bounds were fixed.
```

Validated example:

```text
Woodwinds: Oboe 1 Only
OrchGate CC Number: 23
Result: CC23 opens the Oboe 1 gate correctly.
```

Important bug fixed:

```text
Old max placeholder section preset ID was 5.
Expanded dropdowns needed larger section-specific ranges.
```

Current section preset ranges:

```text
Woodwinds max preset ID: 19
Brass max preset ID: 16
Percussion max preset ID: 8
Strings max preset ID: Preset::tutti
```

---

## Phase 1I — Initial Multi-Track Template Validation

Status:

```text
Informally validated in Bitwig.
```

Confirmed workflow:

```text
Same MIDI clip pasted into multiple orchestral tracks.
Bitwig Randomize device used per track.
Randomized keyswitches used per track.
OrchNoteMapper keeps material in range.
OrchGate controls participation.
OrchConductor flips orchestral combinations manually.
```

Observed result:

```text
Beautiful and musically useful accidents occur.
The system behaves like a long-term generative orchestration environment.
```

Conclusion:

```text
The three-plugin workflow is musically fruitful and should be developed further.
```

---

# 6. Current Stable Workflow

Recommended track chain:

```text
MIDI source / clip / generator / randomizer
        ↓
OrchNoteMapper
        ↓
OrchGate
        ↓
Instrument
```

Recommended conductor chain:

```text
OrchConductor on a conductor/control track
        ↓
MIDI routed to orchestral tracks / gate instances
```

Typical Bitwig template strategy:

```text
1. Create one track per orchestral instrument.
2. Assign each track an OrchNoteMapper preset.
3. Add OrchGate after OrchNoteMapper.
4. Set each OrchGate CC Number to the track’s assigned full-score CC.
5. Paste or route related MIDI material to many tracks.
6. Use OrchConductor to select which instruments participate.
```

This enables:

```text
manual orchestration switching
generative orchestral textures
randomized per-track variation
controlled density
sectional contrast
instrumental holes
doublings and color shifts
```

---

# 7. Current Development Conclusion

The project has crossed from:

```text
plugin prototype
```

to:

```text
usable orchestration instrument
```

The next major development should not be more basic gating.

The next major development should be:

```text
OrchConductor Combi Preset Engine
```

---

# 8. Revised Strategic Direction

The future of OrchConductor is:

```text
an orchestration-mask engine
```

A combi preset is a full-score participation mask:

```text
For each instrument CC:
    0   = inactive
    64  = secondary / ghost / threshold-dependent participation
    127 = active
```

Examples:

```text
Dark Low Choir
Silver Shimmer
Herrmann Horn Knives
Warm Strings + Horns
Pointillist Winds
Sparse Extremes
English Horn Lament
Full Orchestra
Low Orchestra
Chamber Orchestra
```

A combi preset does not merely choose a section dropdown.

It defines a full orchestral state.

---

# 9. Next Milestone: Phase 2A — Built-In Combi Preset Engine

## Goal

Enable the existing Combi Preset dropdown and add a curated set of built-in orchestration combinations.

## Core behavior

```text
Combi Preset = full CC20–CC54 participation map.
```

The plugin should support two control modes:

```text
Manual Sections
    Current behavior.
    Woodwinds / Brass / Percussion / Strings dropdowns determine output.

Combi Preset
    Selected combi preset determines the full CC map directly.
```

## UI behavior

The Combi Preset dropdown should include:

```text
Manual Sections
[Utility] All Off
[Utility] Full Orchestra
[Utility] Chamber Orchestra
[Utility] Full Strings
[Utility] Full Woodwinds
[Utility] Full Brass
[Utility] Low Orchestra
[Utility] High Orchestra
[Romantic] Warm Strings + Horns
[Cinematic] Dark Trailer Bed
[Herrmann] Horn Knives
[Modernist] Pointillist Winds
[Shimmer] Silver Shimmer
...
```

When a combi preset is selected:

```text
OrchConductor sends the corresponding full CC map.
Status line shows the active combi preset.
```

Recommended behavior:

```text
If the user edits any section dropdown manually, the Combi Preset dropdown returns to Manual Sections.
```

This keeps the interaction intuitive.

---

# 10. Suggested Phase 2A Initial Built-In Library

Start with a compact, reliable library of 24 presets.

Do not begin with 200 presets.

First prove the engine.

```text
000 Manual Sections

001 [Utility] All Off
002 [Utility] Full Orchestra
003 [Utility] Full Orchestra No Percussion
004 [Utility] Chamber Orchestra
005 [Utility] Full Strings
006 [Utility] Full Woodwinds
007 [Utility] Full Brass
008 [Utility] Full Winds
009 [Utility] High Orchestra
010 [Utility] Low Orchestra
011 [Utility] Middle Orchestra

012 [Romantic] Warm Strings + Horns
013 [Romantic] Oboe + Strings
014 [Romantic] Flute + Violins
015 [Romantic] Bassoon + Celli
016 [Romantic] Horn Choir + Strings

017 [Cinematic] Heroic Brass + Strings
018 [Cinematic] Dark Trailer Bed
019 [Cinematic] High Winds Shimmer
020 [Cinematic] Epic Low Pulse

021 [Herrmann] Low Reeds
022 [Herrmann] Horn Knives
023 [Herrmann] Psycho Strings
024 [Herrmann] Suspense Winds

025 [Modernist] Pointillist Winds
026 [Modernist] Sparse Extremes
027 [Shimmer] Silver Shimmer
028 [Solo] English Horn Lament
```

The exact number may be adjusted during implementation.

---

# 11. Example Combi Definitions

## Full Orchestra

```text
All mapped instruments active.
```

Targets:

```text
CC20–CC48 = 127
CC50–CC54 = 127
CC49 reserved, not used
```

---

## Chamber Orchestra

```text
Flute 1
Oboe 1
Clarinet 1
Bassoon 1
Horn 1
Violin I
Violin II
Viola
Cello
Double Bass
```

Musical use:

```text
Classical chamber-orchestra texture.
Good default reduced ensemble.
```

---

## Warm Strings + Horns

```text
Horn 1
Horn 2
Horn 3
Horn 4
Violin I
Violin II
Viola
Cello
Double Bass
```

Musical use:

```text
Reliable romantic/cinematic warmth.
Good for harmonic beds and noble textures.
```

---

## Oboe + Strings

```text
Oboe 1
Violin I
Violin II
Viola
Cello
```

Musical use:

```text
Classic lyrical color.
Good for soloistic melodic emergence over a string body.
```

---

## Bassoon + Celli

```text
Bassoon 1
Bassoon 2
Cello
Double Bass optional
```

Musical use:

```text
Dark melodic support.
Useful for comic, melancholic, or low lyrical material.
```

---

## Heroic Brass + Strings

```text
Horns
Trumpets
Trombones
Full Strings
Timpani optional
```

Musical use:

```text
Cinematic heroic statement.
Dense but reliable.
```

---

## Dark Trailer Bed

```text
Bass Clarinet
Contrabassoon
Bass Trombone
Tuba
Cello
Double Bass
Timpani
```

Musical use:

```text
Low threat, trailer tension, dark foundation.
```

---

## High Winds Shimmer

```text
Piccolo
Flute 1
Flute 2
Violin I
Violin II
Glockenspiel
Vibraphone
Tubular Bells optional
```

Musical use:

```text
Bright high-register shimmer.
Useful for magical or suspended textures.
```

---

## Herrmann Low Reeds

```text
Bass Clarinet
Bassoon 1
Bassoon 2
Contrabassoon
Cello
Double Bass
```

Musical use:

```text
Suspenseful low reed/low string color.
Bernard Herrmann-inspired darkness.
```

---

## Herrmann Horn Knives

```text
Horn 1
Horn 2
Horn 3
Horn 4
Viola
Cello
```

Musical use:

```text
Stark mid-register suspense and stabbing harmonic blocks.
```

---

## Herrmann Psycho Strings

```text
Violin I
Violin II
Viola
Cello
```

Musical use:

```text
Tense string-only texture.
No basses.
Useful for sharp, nervous, high/mid string writing.
```

---

## Pointillist Winds

```text
Piccolo
Oboe 1
Clarinet 1
Bassoon 1
```

Musical use:

```text
Sparse modernist woodwind points.
Good for atonal or fragmented textures.
```

---

## Sparse Extremes

```text
Piccolo
Contrabassoon
Trumpet 1
Bass Trombone
Violin I
Double Bass
Xylophone
```

Musical use:

```text
Wide registral gaps.
Useful for modernist, strange, or exposed orchestration.
```

---

## Silver Shimmer

```text
Piccolo
Flute 1
Flute 2
Violin I
Violin II
Glockenspiel
Vibraphone
Tubular Bells
```

Musical use:

```text
Bright, glassy, luminous texture.
```

---

## English Horn Lament

```text
English Horn
Viola
Cello
Double Bass optional
```

Musical use:

```text
Dark lyrical solo color.
Good for lamenting, pastoral, or lonely material.
```

---

# 12. Phase 2B — Automatable Combi Preset Index

## Goal

Expose a host-automatable parameter:

```text
Combi Preset Index
```

This allows Bitwig to control OrchConductor using:

```text
automation lanes
clip automation
LFO modulators
Step modulators
Random modulators
Sample and Hold
Button grids
Note Grid / CC sources
```

This creates the desired:

```text
conductor of the conductor
```

## Required behavior

The parameter should be quantized:

```text
continuous host parameter
    ↓
rounded integer preset index
    ↓
if index changed, load preset and send CC map
```

This prevents unstable behavior during smooth automation.

## Important design note

If a Bitwig modulator moves smoothly from preset 10 to 30, the plugin may pass through many intermediate preset numbers.

This can be musically useful or chaotic.

Possible future solutions:

```text
quantization
sample-and-hold
minimum time between changes
host-synced update grid
category-limited selection
```

For Phase 2B, simple integer quantization is sufficient.

---

# 13. Phase 2C — External JSON Combi Library

## Goal

Move combi definitions out of hardcoded C++ and into an editable JSON library.

## Reason

An external library allows:

```text
large curated preset collections
user expansion
custom artistic names
experimental masks
sharing libraries between projects
future categorization and filtering
```

## Recommended JSON style

Use musical target IDs, not raw CC numbers.

Example:

```json
{
  "libraryVersion": 1,
  "presets": [
    {
      "id": "dark_low_choir",
      "name": "Dark Low Choir",
      "category": "Dark",
      "mood": ["ominous", "dense", "low"],
      "description": "Bass clarinet, bassoons, contrabassoon, low brass, low strings, and timpani.",
      "default": 0,
      "targets": {
        "bass_clarinet": 127,
        "bassoon_1": 127,
        "bassoon_2": 127,
        "contrabassoon": 127,
        "trombone_1": 127,
        "trombone_2": 127,
        "bass_trombone": 127,
        "tuba": 127,
        "viola": 64,
        "cello": 127,
        "double_bass": 127,
        "timpani": 64
      }
    }
  ]
}
```

The plugin maps IDs internally:

```text
oboe_1        -> CC23
horn_4        -> CC35
double_bass   -> CC54
```

This keeps the library readable and resilient.

---

# 14. Phase 2D — Save / Export Current Combi

## Goal

Allow custom experimental presets to be captured and added to the user library.

## Recommended first implementation

Start with:

```text
Copy Current Combi JSON
```

This button copies the current full CC state to the clipboard as a JSON preset fragment.

The user can then paste it into:

```text
OrchConductor_UserLibrary.json
```

This avoids early file permission and path issues.

## Later implementation

Add:

```text
Save Current Combi...
Load User Library...
Reload Library
```

Potential storage locations:

```text
user documents folder
plugin support folder
project-local folder
manually selected JSON file
```

---

# 15. Phase 2E — Travel / Journey Modes

## Goal

Allow OrchConductor itself to move through preset states over time.

Possible modes:

```text
Off
Forward
Backward
Random
Weighted Random
Category Random
Brownian / Neighbor Walk
Sparse-to-Dense
Dark-to-Bright
Call/Response
Scene Steps
```

Possible rates:

```text
1/4
1/2
1 bar
2 bars
4 bars
8 bars
free seconds
```

This phase should come after Bitwig automation/modulation has been tested.

Reason:

```text
Bitwig may already provide most travel behavior through modulators.
Internal travel should only be added after clear musical needs appear.
```

---

# 16. Later Possible Features

## 16.1 OrchGate Optional Template Target Helper

Add an optional dropdown to OrchGate:

```text
Template Target:
Custom
Piccolo CC20
Flute 1 CC21
Flute 2 CC22
...
Double Bass CC54
```

Selecting a target simply sets the CC Number.

Important:

```text
Raw CC Number remains primary.
Custom remains available.
OrchGate stays instrument-agnostic.
```

This is only a usability helper.

---

## 16.2 OrchGate Expression Fade Mode

Original roadmap idea:

```text
When gate closes:
    fade CC11 or CC7 toward 0
    optionally send note-offs after fade time
```

Potential use:

```text
soft orchestral fades
long textures
less abrupt participation changes
```

Not urgent because current No New Notes behavior is already musical.

---

## 16.3 OrchNoteMapper Full-Score Aliases

Possible future addition:

```text
Flute 1
Flute 2
Horn 1
Horn 2
Horn 3
Horn 4
Trumpet 1
...
```

These would share existing instrument range logic.

Not urgent because:

```text
Bitwig device presets can already provide customized labels.
The current mapper is functional.
Changing it now offers mostly cosmetic benefit.
```

---

## 16.4 Harp Implementation

CC49 is reserved for Harp.

Harp likely requires custom logic rather than simple sustained-gate behavior.

Future considerations:

```text
range handling
hand-shape constraints
pedal / scale constraints
arpeggio behavior
glissando behavior
wrapping or clamping rules
```

Do not implement Harp as a simple ordinary gate until the musical behavior is defined.

---

## 16.5 Active Players Display

Possible OrchConductor UI feature:

```text
Show currently active instruments.
Show active count.
Show active sections.
Show current combi mask.
```

This could replace static map information with useful real-time feedback.

---

# 17. Technical Priorities

Highest priority remains:

```text
Prevent stuck notes.
```

For OrchGate:

```text
track active notes per channel/note
send note-offs safely when needed
handle transport stop/reset
handle all-notes-off
preserve keyswitch pass-through behavior
```

For OrchConductor:

```text
send complete CC states when changing presets
avoid partial stale states
ensure All Off sends all mapped CCs to 0
preserve CC49 reserved behavior
```

For automatable combi presets:

```text
quantize parameter changes
avoid excessive CC spam if index has not changed
send full CC map only when necessary
maintain deterministic preset recall
```

---

# 18. Recommended Development Order From Current State

```text
1. Phase 2A — Built-in Combi Preset Engine
2. Phase 2B — Automatable Combi Preset Index
3. Phase 2C — External JSON Combi Library
4. Phase 2D — Save / Export Current Combi
5. Phase 2E — Travel / Journey Modes
6. Optional OrchGate Template Target Helper
7. Optional OrchGate Expression Fade
8. Optional OrchNoteMapper full-score aliases
9. Harp-specific implementation
```

Do not prioritize:

```text
true plugin-to-plugin master/slave communication
complex cross-instance messaging
large OrchNoteMapper refactors
per-track probability engines inside OrchGate
```

The validated MIDI CC architecture is simpler and better.

---

# 19. Recommended Branch / Tag Naming

Next branches:

```text
phase-2A-combi-preset-engine
phase-2B-automatable-combi-index
phase-2C-json-combi-library
phase-2D-save-export-combis
phase-2E-combi-travel-modes
```

Possible validation tags:

```text
phase-2A-combis-validated
phase-2B-combi-automation-validated
phase-2C-json-library-validated
```

---

# 20. Current Summary

The system currently consists of:

```text
OrchNoteMapper:
    makes MIDI playable

OrchGate:
    decides whether a track speaks

OrchConductor:
    decides who participates globally
```

The current validated workflow is:

```text
same or related MIDI material across many tracks
+
per-track randomization
+
per-track range mapping
+
per-track gates
+
global conductor CC control
=
a playable generative orchestration environment
```

The next major goal is:

```text
Turn OrchConductor into a curated orchestration-mask engine.
```

The next practical milestone is:

```text
Phase 2A: Built-In Combi Preset Engine
```

The long-term creative goal is:

```text
A library of named orchestral states:
    dark
    shimmer
    sparse
    dense
    romantic
    cinematic
    Herrmann-like
    modernist
    soloistic
    chamber
    tutti

that can be selected, automated, randomized, traveled through, saved, and expanded.
```

This is the current direction of the project.
