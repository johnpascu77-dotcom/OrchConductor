\# OrchConductor Phase 5C - Processor Runtime Catalog Payload Equivalence Probe



\## Status



Phase 5C adds a processor-side runtime catalog payload equivalence probe.



The probe is diagnostic-only and passive. The plugin still uses hardcoded processor behavior for preset application and MIDI output.



\## Purpose



Phase 5C checks whether source-backed runtime catalog payloads match selected hardcoded processor payloads.



This phase does not adopt runtime catalog payloads for MIDI generation.



\## Feature Gate



```text

ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS

```



\## Fallback-Aware Behavior



When runtime JSON is OFF:



```text

Payload equivalence probe does not run.

Payload equivalence probe does not report pass.

Payload equivalence probe reports fallback/inactive block.

Hardcoded behavior remains authoritative.

```



When runtime JSON is ON but runtime source is unavailable or malformed:



```text

Runtime catalog authority candidate may be ready via fallback metadata.

Payload equivalence probe does not run against fallback metadata.

Payload equivalence probe reports fallback block.

Hardcoded behavior remains authoritative.

```



When runtime JSON is ON and catalog is source-backed:



```text

Payload equivalence probe runs.

Selected hardcoded sentinel payloads are compared against runtime catalog payloads.

Mismatch fails developer checks.

Hardcoded behavior remains authoritative.

```



\## Sentinel Payloads



```text

strings preset index 1: Low Strings

combi preset index 1: Solo English Horn Lament

```



\## Passive Boundary



Phase 5C does not allow runtime catalog data to drive:



```text

Preset application

MIDI CC generation

Editor combo-box population

State serialization

Plugin parameter behavior

```



\## MIDI Safety



The MIDI regression check confirms that the payload equivalence probe does not alter hardcoded MIDI behavior.



\## Completion Criteria



```text

Default processor JSON probe check passes.

Default processor MIDI regression check passes.

Explicit OFF processor checks pass.

Explicit ON processor checks pass.

Plugin target builds.

Runtime catalog check still passes.

Forbidden files remain untouched.

Working tree is clean after commit.

Phase is tagged.

```



\## Recommended Commit



```text

Add processor runtime catalog payload equivalence probe

```



\## Recommended Tag



```text

phase-5C-processor-runtime-catalog-payload-equivalence-probe

```



