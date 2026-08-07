# OrchConductor Phase 5G Runtime Catalog Authority Trial Diagnostic

## Purpose

Phase 5G adds a processor-side diagnostic that can verify selected runtime catalog payloads against known factory-authority sentinel values.

The diagnostic is intentionally passive. It does not change MIDI routing, preset selection, hardcoded preset behavior, or runtime JSON authority. The hardcoded processor behavior remains authoritative.

## Feature Gates

Phase 5G is controlled by:

- `ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS`
- `ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL`

The authority trial can only run when runtime JSON presets are enabled and the runtime preset catalog is source-backed, ready, and has the expected factory shape.

## Runtime States

### Runtime JSON disabled

When `ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS` is disabled:

- the authority trial does not run
- the authority trial does not pass
- the authority trial is reported as blocked
- the diagnostic explains that runtime JSON presets are disabled

### Runtime JSON enabled but catalog fallback required

When runtime JSON presets are enabled but the runtime catalog requires fallback or is not source-backed:

- the authority trial does not run
- the authority trial does not pass
- the authority trial is reported as blocked
- the diagnostic explains that the catalog fallback/source-backed condition blocked the trial

### Runtime JSON enabled, catalog source-backed, authority trial disabled

When runtime JSON presets are enabled and the catalog is source-backed, but `ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL` is disabled:

- the authority trial does not run
- the authority trial does not pass
- the authority trial is reported as blocked
- the diagnostic explains that the authority trial feature gate is disabled

### Runtime JSON enabled, catalog source-backed, authority trial enabled

When both feature gates are enabled and the catalog is source-backed:

- the authority trial runs
- the sentinel check must pass
- the authority trial is not blocked
- the diagnostic reports pass/fail status

## Sentinel Coverage

Phase 5G checks selected factory-authority sentinel payloads:

### Strings preset 8: Low Strings

Expected values:

| CC | Value |
| --- | ---: |
| 50 | 0 |
| 51 | 0 |
| 52 | 64 |
| 53 | 127 |
| 54 | 127 |

### Strings preset 12: Full Strings

Expected values:

| CC | Value |
| --- | ---: |
| 50 | 127 |
| 51 | 127 |
| 52 | 127 |
| 53 | 127 |
| 54 | 127 |

### Combi preset 28: Solo English Horn Lament

Expected values:

| CC | Value |
| --- | ---: |
| 20 | 0 |
| 21 | 0 |
| 22 | 0 |
| 23 | 0 |
| 24 | 0 |
| 25 | 127 |
| 26 | 0 |
| 27 | 0 |
| 28 | 0 |
| 29 | 0 |
| 30 | 0 |
| 31 | 0 |
| 32 | 0 |
| 33 | 0 |
| 34 | 0 |
| 35 | 0 |
| 36 | 0 |
| 37 | 0 |
| 38 | 0 |
| 39 | 0 |
| 40 | 0 |
| 41 | 0 |
| 42 | 0 |
| 43 | 0 |
| 44 | 0 |
| 45 | 0 |
| 46 | 0 |
| 47 | 0 |
| 48 | 0 |
| 49 | 0 |
| 50 | 0 |
| 51 | 0 |
| 52 | 127 |
| 53 | 127 |
| 54 | 64 |

## Processor Accessors

Phase 5G exposes these processor diagnostics:

```cpp
bool wasRuntimeCatalogAuthorityTrialRun() const;
bool didRuntimeCatalogAuthorityTrialPass() const;
bool wasRuntimeCatalogAuthorityTrialBlocked() const;
juce::String getRuntimeCatalogAuthorityTrialDiagnostic() const;
```

## Probe Tool Verification

`OrchConductorProcessorJsonProbeCheck` verifies that:

- the diagnostic message is present
- the trial is inactive and blocked when runtime JSON is disabled
- the trial is inactive and blocked when the catalog requires fallback
- the trial is inactive and blocked when the Phase 5G authority-trial feature gate is disabled
- the trial runs and passes when both runtime JSON and the authority-trial feature gate are enabled

## Validated Command

The authority-on configuration was validated with:

```powershell
cmake -S . -B build-phase5G-authority-on -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON -DORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL=ON
cmake --build build-phase5G-authority-on --target OrchConductorProcessorJsonProbeCheck --config Debug
.\build-phase5G-authority-on\OrchConductorProcessorJsonProbeCheck_artefacts\Debug\OrchConductorProcessorJsonProbeCheck.exe
```

Result:

```text
PASS
```

## Safety Notes

Phase 5G remains diagnostic-only.

It does not:

- change preset authority
- replace hardcoded processor presets
- route MIDI from runtime JSON
- change editor behavior
- change processor state serialization
