# OrchConductor Phase 1G Validation Note

Date: 2026-08-03

## Status

OrchConductor Phase 1G was built, deployed, loaded in Bitwig, and functionally validated.

## Confirmed behavior

Phase 1G implements the full-score MIDI CC gate map in top-down orchestral order.

```text
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

CC43  Timpani
CC44  Glockenspiel
CC45  Xylophone
CC46  Marimba
CC47  Vibraphone
CC48  Tubular Bells

CC49  Reserved for Harp

CC50  Violin I
CC51  Violin II
CC52  Viola
CC53  Cello
CC54  Double Bass
```

## UI cleanup

The previous permanent lower “Selected Output” tables were removed from the main UI.

A compact `Show MIDI Map` button now opens a reference popup containing the full CC map.

The lower UI area is now reserved for future orchestral combo presets / active players / Divisimate-style controls.

## Confirmed Bitwig UI

```text
Build: Phase 1G
Footer: Phase 1G MIDI Map: Full-score CC20-CC54 | CC49 reserved for Harp
Status: Selected strings preset: All Off | Full-score MIDI gates active | Harp reserved | Auto-send: Off
```

## Result

```text
OrchConductor Phase 1G is functionally valid.
```

## Recommended stable tag

```text
phase-1G-validated
```
