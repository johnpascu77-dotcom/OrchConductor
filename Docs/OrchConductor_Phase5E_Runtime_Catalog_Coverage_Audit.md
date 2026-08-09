# OrchConductor Phase 5E — Runtime Catalog Coverage Audit

## Purpose

Phase 5E adds a passive processor-side runtime catalog coverage audit.

The audit verifies that the runtime preset catalog exposes the expected factory
catalog surface before any future work considers runtime JSON as an authority
candidate.

This phase does not change preset selection, MIDI output, fallback behavior,
state serialization, or UI behavior.

## Runtime JSON OFF behavior

When runtime JSON presets are disabled, the audit remains inactive.

Expected diagnostic:

```text
Runtime catalog coverage audit is inactive because runtime JSON presets are disabled.
```

Expected state:

```text
run: false
pass: false
blocked by fallback: true
```

## Runtime JSON ON behavior

When runtime JSON presets are enabled and the runtime catalog is source-backed,
the audit runs passively against the source-backed catalog.

Expected diagnostic:

```text
Runtime catalog coverage audit passed for expected factory catalog shape.
```

Expected state:

```text
run: true
pass: true
blocked by fallback: false
```

If the runtime catalog requires fallback or is not source-backed, the audit is
blocked and remains passive.

## Coverage checked

The audit verifies factory section preset ID coverage:

```text
woodwinds:  0 through 19
brass:      0 through 16
percussion: 0 through 8
strings:    0 through 12
```

The audit verifies factory combi preset ID coverage:

```text
combi: 0 through 28
```

The audit also checks runtime payload safety:

```text
CC number: 0 through 127
Value:     0 through 127
```

For expanded combi payloads, the audit verifies the reserved CC49 convention:

```text
CC49 == 0
```

## Authority boundary

Hardcoded MIDI behavior remains authoritative in Phase 5E.

The runtime catalog coverage audit is diagnostic-only and feature-gated. It is
intended to increase confidence in the runtime catalog shape without changing
runtime behavior.