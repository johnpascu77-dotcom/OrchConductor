# OrchConductor Phase 1A.1 Validation Note

Date: 2026-08-03

## Status

OrchConductor Phase 1A.1 was built, deployed, loaded in Bitwig, and functionally validated.

## New features

```text
Send on Preset Change checkbox
Dedicated Send All Off button
```

## Confirmed behavior

Manual preset sending still works.

Send on Preset Change works.

Send All Off works.

The plugin continues to send CC messages while Bitwig is stopped.

Multiple OrchGate instances respond correctly.

## Confirmed example

String Quartet preset with Send on Preset Change enabled:

```text
CC20 = 127 → Violin I open
CC21 = 127 → Violin II open
CC22 = 127 → Viola open
CC23 = 127 → Cello open
CC24 = 0   → Double Bass closed
```

Observed in Bitwig:

```text
CC20 = 127 >= 64 → gate open
CC24 = 0 < 64    → gate closed
```

## Result

```text
OrchConductor Phase 1A.1 is functionally valid.
```
