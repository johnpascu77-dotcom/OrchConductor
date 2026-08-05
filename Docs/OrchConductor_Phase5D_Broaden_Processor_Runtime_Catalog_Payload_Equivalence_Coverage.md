# OrchConductor Phase 5D - Broaden Processor Runtime Catalog Payload Equivalence Coverage

## Purpose

Expand the processor-side runtime catalog payload equivalence probe from two sentinels to broader catalog coverage.

This remains diagnostic-only. Hardcoded MIDI behavior remains authoritative.

## Confirmed Correction

Phase 5D corrects the Solo English Horn Lament runtime catalog combi sentinel from index/factory ID 1 to index/factory ID 28.

The embedded factory JSON defines:

```json
{
  "name": "[Solo] English Horn Lament",
  "factoryId": 28
}
```

## Expanded Sentinel Coverage

Section preset sentinels:

- Strings preset 8: Low Strings
- Woodwinds preset 19: Full Woodwinds
- Brass preset 16: Full Brass
- Percussion preset 8: Full Melodic Percussion
- Strings preset 12: Full Strings

Combi preset sentinels:

- Combi preset 2: [Utility] Full Orchestra
- Combi preset 28: [Solo] English Horn Lament

## Boundary

The probe is passive and feature-gated by runtime JSON availability.

It does not alter hardcoded preset selection, hardcoded MIDI CC output, UI preset behavior, or fallback behavior.

## Status

Phase 5D broadens payload equivalence coverage across current runtime catalog sections and combi payloads.
