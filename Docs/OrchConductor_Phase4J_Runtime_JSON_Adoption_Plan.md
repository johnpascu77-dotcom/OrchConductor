# OrchConductor Phase 4J - Runtime JSON Adoption Plan

## Status

Phase 4J is a documentation-only phase.

No code is changed.

No build configuration is changed.

No schemas are changed.

No tools are changed.

No examples are changed.

No scripts are changed.

Runtime JSON preset adoption remains disabled.

Hardcoded factory presets remain authoritative.

## Purpose

Phase 4J defines the adoption plan for moving from hardcoded factory preset data toward JSON-backed factory preset data.

The plan is intentionally conservative.

The project has already established:

```text
Passive JSON infrastructure.
Factory JSON schema validation.
Factory JSON parity verification.
Embedded factory JSON resource access.
RuntimePresetSource boundary.
Processor construction-time JSON probe diagnostics.
MIDI regression verification with the probe OFF and ON.
```

Phase 4J freezes rollout rules before any plugin behavior begins to read preset names or values from JSON.

## Current Runtime Contract

At the end of Phase 4I:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS defaults to OFF.
OFF builds bypass runtime JSON preset loading.
ON builds may load embedded JSON through RuntimePresetSource.
Loaded JSON is diagnostic only.
Hardcoded preset names remain authoritative.
Hardcoded preset values remain authoritative.
Hardcoded MIDI output remains authoritative.
Editor population remains hardcoded.
Plugin state serialization remains unchanged.
Audio-thread behavior remains unchanged.
```

## Non-Negotiable Adoption Principles

Runtime JSON adoption must follow these principles:

```text
Fallback first.
Feature gated.
No disk I/O.
No audio-thread JSON parsing.
No audio-thread allocation introduced by JSON adoption.
No plugin state breakage.
No MIDI output change without explicit parity approval.
No UI label change without explicit label parity approval.
No preset ID reorder.
No silent fallback ambiguity.
No fatal failure from malformed or unavailable embedded JSON.
```

## Authoritative Fallback Rules

Hardcoded preset data remains the fallback authority until a later phase explicitly promotes JSON to read-only authority.

Fallback is required when any of the following are true:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS is OFF.
Embedded JSON data is unavailable.
Embedded JSON data is empty.
Embedded JSON cannot be parsed.
JSON schema validation fails.
JSON preset counts do not match expected hardcoded counts.
JSON section IDs do not match expected hardcoded section IDs.
JSON preset IDs do not match expected hardcoded preset IDs.
JSON combi IDs do not match expected hardcoded combi IDs.
JSON CC mappings do not match expected hardcoded CC mappings.
JSON reserved CC rules are violated.
JSON parity checks fail.
RuntimePresetSource reports an unsuccessful load.
```

Fallback behavior must be:

```text
Non-fatal.
Deterministic.
Diagnostic.
Non-audio-thread.
Invisible to MIDI output unless explicitly surfaced in a developer check.
```

## Feature Gate Policy

The existing feature gate remains the only runtime JSON preset adoption switch:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
```

Policy:

```text
OFF is the default.
OFF means hardcoded behavior only.
ON may enable JSON loading and diagnostics.
ON does not automatically mean JSON-backed behavior.
Each JSON-backed read path must be separately introduced, verified, and documented.
```

A future phase may add narrower internal switches if needed, but they must remain subordinate to:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
```

## Runtime Boundary Ownership

The runtime JSON adoption boundary remains:

```text
RuntimePresetSource
```

Responsibilities:

```text
Access embedded factory JSON.
Parse factory JSON.
Validate loaded data.
Expose loaded data through stable read-only models.
Expose diagnostic status.
Report whether hardcoded fallback is required.
Avoid disk I/O.
Avoid audio-thread use.
Avoid owning UI or MIDI policy.
```

Non-responsibilities:

```text
RuntimePresetSource must not emit MIDI.
RuntimePresetSource must not mutate plugin state.
RuntimePresetSource must not own preset selection.
RuntimePresetSource must not directly populate editor controls.
RuntimePresetSource must not parse from arbitrary user file paths.
RuntimePresetSource must not perform work from processBlock().
```

## Processor Ownership

The processor remains responsible for:

```text
Preset selection state.
Plugin state serialization.
MIDI emission.
Send request handling.
Audio callback behavior.
Exposing safe diagnostics for developer checks.
```

During early adoption, the processor may hold a successfully loaded runtime preset source, but must not use it for MIDI output until all required gates pass.

## Editor Ownership

The editor remains responsible for:

```text
Displaying current controls.
Populating combo boxes.
Handling user interactions.
Calling processor selection APIs.
```

The editor must not parse JSON.

The editor must not directly access embedded binary JSON.

If UI label adoption occurs, the editor should read labels only through processor-owned or catalog-adapter-owned read-only accessors that are safe and already initialized before editor construction.

## Required Gates Before Any Behavior Switch

Before any behavior switches from hardcoded data to JSON-backed data, all of the following must pass:

```text
Factory JSON schema validation.
Factory JSON hardcoded parity verification.
Embedded JSON accessor verification.
RuntimePresetSource load verification.
RuntimePresetSource parity verification.
Processor construction-time JSON probe verification.
Processor MIDI regression verification.
Default/OFF build verification.
Explicit OFF build verification.
Explicit ON build verification.
Default plugin target build.
Explicit ON plugin target build.
```

The behavior switch must also include a new targeted verification phase for the specific read path being switched.

Examples:

```text
UI label switch requires UI label parity verification.
Preset value switch requires MIDI output parity verification.
Combi switch requires combi MIDI parity verification.
State serialization switch requires state compatibility verification.
```

## Adoption Sequence

Recommended adoption sequence:

```text
1. Documentation-only adoption plan.
2. Read-only catalog adapter skeleton.
3. Catalog adapter parity checks.
4. Processor-owned catalog accessor, still non-authoritative.
5. UI label read-path trial behind feature gate.
6. UI label parity verification.
7. UI label adoption behind feature gate.
8. Preset value read-path trial behind feature gate.
9. MIDI parity verification.
10. Preset value adoption behind feature gate.
11. Extended regression matrix.
12. Consider making JSON-backed read paths default only after release confidence.
```

## What May Read JSON First

The first permitted production read path should be low-risk and non-MIDI-authoritative.

Preferred first read path:

```text
UI label catalog access
```

Rationale:

```text
UI labels are visible and easy to compare.
UI labels do not directly emit MIDI.
UI label differences can be detected in deterministic checks.
Failures can fall back to hardcoded labels without audio impact.
```

Even for labels, adoption must be feature-gated and fallback-safe.

## What Must Remain Hardcoded Until Later

The following must remain hardcoded until additional verification exists:

```text
MIDI CC values.
Combi preset values.
Preset ID ordering.
Section ordering.
Reserved CC behavior.
Plugin state serialization.
Default selected preset IDs.
Automation-facing parameter identity if added later.
```

MIDI values are the highest-risk adoption area and must be switched only after exhaustive parity checks.

## Plugin State Compatibility

JSON adoption must not break existing saved sessions.

Rules:

```text
Stored state IDs must remain stable.
Section IDs must remain stable.
Preset IDs must remain stable.
Combi IDs must remain stable.
Unknown or out-of-range restored IDs must retain current fallback behavior.
Changing display labels must not change stored IDs.
Changing JSON order must not change restored behavior.
```

If future JSON includes aliases, display ordering, or deprecated presets, those concepts must be additive and must not alter existing ID semantics.

## UI Label Adoption Strategy

UI label adoption should proceed as:

```text
Add read-only catalog accessors.
Verify hardcoded label parity.
Add developer UI catalog verification target.
Enable editor label reads only when runtime JSON is loaded and parity-valid.
Fallback to hardcoded labels otherwise.
Keep preset IDs and combo item IDs unchanged.
Verify OFF and ON behavior.
Document screenshots or text snapshots if needed.
```

The editor should not be responsible for deciding JSON validity.

Validity should be represented by a processor/catalog-level readiness flag.

## Preset Value Adoption Strategy

Preset value adoption is higher risk and should occur only after UI label adoption is stable.

Requirements:

```text
JSON-backed values must be fully materialized outside processBlock().
processBlock() must read from immutable/precomputed data only.
The MIDI event set must remain identical for equivalent presets.
CC order must remain deterministic.
Reserved CC49 must always emit 0.
MIDI channel must remain 1 unless a later explicit feature changes it.
Send count must remain 35 unless a later explicit feature changes it.
All OFF and ON regression checks must pass.
```

The adoption phase must include comparison of:

```text
Every section preset.
Every combi preset.
Send All Off.
Manual section mode.
Combi override mode.
Request consumption behavior.
No-request behavior.
```

## Threading And Realtime Safety

JSON adoption must observe:

```text
No JSON parsing in processBlock().
No embedded JSON access in processBlock().
No dynamic JSON allocation in processBlock().
No locks introduced in processBlock() for JSON catalog access.
No filesystem access for factory presets.
No network access.
No blocking validation work on the audio thread.
```

Acceptable initialization locations:

```text
Processor construction.
Explicit non-audio-thread initialization step.
Developer verification executable startup.
```

Any future mutable catalog refresh must be explicitly designed and must not occur while audio processing depends on mutable data.

## Failure Handling And Diagnostics

Failure handling must be:

```text
Non-fatal.
Human-readable.
Developer-verifiable.
Fallback-safe.
```

Diagnostics should include:

```text
Feature gate state.
Embedded JSON availability.
Parse success or failure.
Schema validation success or failure.
Parity validation success or failure.
Fallback-required state.
Short diagnostic message.
```

Diagnostics must not:

```text
Allocate or format strings in processBlock().
Change MIDI output.
Change preset selection.
Prevent plugin construction.
Prevent editor construction.
```

## Rollback Strategy

Rollback must remain simple at every adoption stage.

Immediate rollback mechanisms:

```text
Set ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF.
Disable the specific JSON-backed read path if a narrower internal switch exists.
Fallback to hardcoded data when RuntimePresetSource is not ready.
Revert the adoption phase commit if necessary.
```

Every adoption phase must preserve the hardcoded path.

No phase may delete hardcoded preset data until a much later explicit removal phase, after repeated release validation.

## OFF Build Test Matrix

Default/OFF builds must verify:

```text
Plugin target builds.
Developer verification targets build.
Runtime JSON loading is bypassed.
Fallback is required.
Hardcoded preset library checks pass.
Embedded JSON checks may still verify passive resources if target is built.
Runtime source checks pass according to OFF expectations.
Processor JSON probe check passes according to OFF expectations.
Processor MIDI regression check passes according to OFF expectations.
MIDI output remains identical to the pre-adoption baseline.
```

## ON Build Test Matrix

Explicit ON builds must verify:

```text
Plugin target builds.
Developer verification targets build.
Embedded JSON is accessible.
RuntimePresetSource loads successfully.
Runtime source parity checks pass.
Processor JSON probe check passes according to ON expectations.
Processor MIDI regression check passes according to ON expectations.
Any newly adopted read path passes targeted parity checks.
Fallback is not required only when all validity checks pass.
MIDI output remains identical unless the phase explicitly changes MIDI behavior.
```

## Release Risk Controls

Before enabling any JSON-backed behavior in a release build:

```text
All OFF checks must pass.
All ON checks must pass.
Behavior-specific parity checks must pass.
The rollback path must be documented.
The diagnostic path must be documented.
Saved-session compatibility must be verified.
Known failure modes must be listed.
```

A release should not make JSON-backed MIDI values default until at least one prior version has shipped with JSON loading and diagnostics enabled but non-authoritative, or until equivalent internal validation confidence is documented.

## Forbidden Changes During Adoption

Unless explicitly scoped in a later phase, JSON adoption must not:

```text
Change MIDI CC mappings.
Change MIDI values.
Change MIDI channel.
Change emitted CC count.
Change send request semantics.
Change plugin state format.
Change preset IDs.
Change combi IDs.
Change section IDs.
Change editor control semantics.
Add disk-based factory preset loading.
Add user-editable factory JSON.
Add network access.
```

## Phase 4J Completion Criteria

Phase 4J is complete when:

```text
This adoption plan is added.
No code files are modified.
No CMake files are modified.
No tool files are modified.
No schema files are modified.
No example files are modified.
No script files are modified.
Working tree is clean after commit.
Phase is tagged.
```

Recommended commit message:

```text
Document runtime JSON adoption plan
```

Recommended tag:

```text
phase-4J-runtime-json-adoption-plan
```

## Recommended Next Phase

Recommended next phase:

```text
Phase 4K: Read-only Runtime Preset Catalog Adapter Skeleton
```

Suggested Phase 4K scope:

```text
Add a read-only adapter that can represent either hardcoded or runtime-loaded catalog data.
Keep hardcoded authoritative.
Keep adapter non-authoritative.
Add developer-only adapter construction checks.
Do not change editor population.
Do not change processBlock().
Do not change MIDI output.
Do not change plugin state.
```
