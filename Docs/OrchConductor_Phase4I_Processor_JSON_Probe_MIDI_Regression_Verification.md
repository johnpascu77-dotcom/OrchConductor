# OrchConductor Phase 4I - Processor JSON Probe MIDI Regression Verification

## Status

Phase 4I adds developer-only MIDI regression verification for the processor construction-time JSON probe introduced in Phase 4H.

Phase 4I does not change processor behavior.

Phase 4I does not change editor behavior.

Phase 4I does not change runtime JSON loader behavior.

Phase 4I does not enable JSON-backed preset adoption.

Hardcoded factory preset behavior remains authoritative.

## Modified Files

```text
CMakeLists.txt
```

## Added Files

```text
Tools/OrchConductorProcessorMidiRegressionCheck.cpp
Docs/OrchConductor_Phase4I_Processor_JSON_Probe_MIDI_Regression_Verification.md
```

## Verification Target

Phase 4I adds:

```text
OrchConductorProcessorMidiRegressionCheck
```

This target constructs `OrchConductorAudioProcessor`, calls `processBlock()`, captures emitted MIDI CC events, and verifies selected hardcoded behavior.

The same target is expected to pass in both:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

## Verified Behavior

The regression check verifies:

```text
No MIDI is emitted when no send request is pending.
Requested sends emit 35 MIDI CC events.
Every emitted CC uses MIDI channel 1.
Expected CC range is emitted: CC20..48, reserved CC49, CC50..54.
Send All Off emits zero values for all emitted CCs.
Reserved CC49 always emits value 0.
Manual section mode remains available.
Manual strings.low_strings emits CC52=64, CC53=127, CC54=127.
Manual full woodwinds emits CC20..31=127.
Manual full brass emits CC32..42=127.
Combi mode overrides manual section presets.
solo.english_horn_lament emits CC25=127, CC52=127, CC53=127, CC54=64.
Send requests are consumed after one processBlock call.
Processor JSON probe diagnostics are present.
Probe diagnostics do not affect MIDI output.
Hardcoded MIDI behavior remains authoritative.
```

## OFF Build Expectations

When built with:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
```

the check verifies that:

```text
The processor JSON probe is not loaded.
The processor JSON probe requires hardcoded fallback.
The hardcoded MIDI regression suite passes.
```

Expected final lines:

```text
[PASS] Processor MIDI regression verification completed successfully with runtime JSON probe OFF.
[PASS] Hardcoded MIDI behavior remains authoritative.
```

## ON Build Expectations

When built with:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

the check verifies that:

```text
The processor JSON probe loads embedded JSON.
The processor JSON probe does not require fallback after successful load.
The hardcoded MIDI regression suite still passes.
```

Expected final lines:

```text
[PASS] Processor MIDI regression verification completed successfully with runtime JSON probe ON.
[PASS] Hardcoded MIDI behavior remains authoritative.
```

## Runtime Non-Adoption

Phase 4I intentionally does not:

```text
Read preset names from JSON.
Read section preset values from JSON.
Read combi preset values from JSON.
Change preset IDs.
Change state serialization.
Change UI labels.
Change processBlock logic.
Change MIDI output.
```

## Realtime Safety

Phase 4I adds only a developer verification executable.

No JSON parsing is added to the audio callback.

No disk I/O is added.

No plugin runtime behavior is changed.

## Relationship To Previous Phases

```text
Phase 4F introduced RuntimePresetSource.
Phase 4G verified runtime boundary parity.
Phase 4H added a processor construction-time JSON probe.
Phase 4I verifies the Phase 4H probe does not affect MIDI behavior.
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4J: Runtime JSON Adoption Plan
```

Suggested Phase 4J scope:

```text
Document the final JSON-backed preset adoption strategy.
Define exact fallback rules.
Define parity gates required before switching any UI/preset source.
Define how JSON-backed data will be exposed without audio-thread parsing.
Define rollback strategy.
Keep implementation unchanged.
```

A more aggressive alternative is:

```text
Phase 4J: JSON-backed read-only preset catalog adapter skeleton
```

That alternative should still avoid changing MIDI output or editor population until a later verified phase.

## Completion Criteria

Phase 4I is complete when:

```text
OrchConductorProcessorMidiRegressionCheck target exists.
Default/OFF MIDI regression check passes.
Explicit OFF MIDI regression check passes.
Explicit ON MIDI regression check passes.
Existing developer checks still build.
Processor/editor/runtime-source files remain untouched.
Default build succeeds.
Explicit ON build succeeds.
The phase is committed and tagged.
```

Recommended tag:

```text
phase-4I-processor-json-probe-midi-regression-verification
```
