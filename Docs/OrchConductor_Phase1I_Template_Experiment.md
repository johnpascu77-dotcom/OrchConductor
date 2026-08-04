# OrchConductor Phase 1I Template Experiment Note

Date: 2026-08-03

## Status

Initial multi-track Bitwig template experiment was successful.

## Confirmed workflow

The same MIDI clip can be pasted across multiple orchestral tracks.

Each track can contain:

```text
Bitwig Randomize
OrchNoteMapper
OrchGate
Instrument
```

OrchNoteMapper keeps randomized/transposed material within practical instrumental ranges.

OrchGate controls participation per track via the Phase 1G/1H CC map.

OrchConductor can manually switch orchestral participation presets across sections.

## Observed musical result

Randomized keyswitches and per-track randomization create useful variation and musical accidents while remaining constrained by instrument mapping.

## Current conclusion

The OrchNoteMapper + OrchGate + OrchConductor system is musically fruitful and suitable for long-term development.

## Next likely direction

Continue practical template testing before changing OrchNoteMapper.

Potential future features:

```text
- OrchConductor combi presets
- OrchGate optional target helper
- OrchNoteMapper full-score aliases, if needed
```
