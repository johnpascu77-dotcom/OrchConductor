# OrchConductor Phase 1A Validation Note

Date: 2026-08-02

## Status

OrchConductor Phase 1A was built, deployed, loaded in Bitwig, and functionally validated.

## Build

Generator:

```text
Visual Studio 18 2026
```

Build command:

```text
cmake --build build --config Release
```

Built VST3:

```text
C:\AudioDev\Repos\OrchConductor\build\OrchConductor_artefacts\Release\VST3\OrchConductor.vst3
```

Deployment target:

```text
C:\AudioDev\ActiveVST3\OrchConductor.vst3
```

## Confirmed behavior

The plugin loads in Bitwig.

Manual preset sending works.

The plugin successfully emits CC messages while Bitwig is stopped.

Multiple OrchGate instances respond correctly to the emitted CC values.

## Phase 1A CC Map

```text
CC20 = Violin I
CC21 = Violin II
CC22 = Viola
CC23 = Cello
CC24 = Double Bass
```

## Confirmed example

String Quartet preset:

```text
CC20 = 127
CC21 = 127
CC22 = 127
CC23 = 127
CC24 = 0
```

Observed OrchGate responses included:

```text
CC21 = 127 >= 64 → gate open
CC22 = 127 >= 64 → gate open
CC24 = 0 < 64   → gate closed
```

## Result

```text
OrchConductor Phase 1A is functionally valid.
```

## Stable tag recommendation

```text
phase-1A-validated
```
