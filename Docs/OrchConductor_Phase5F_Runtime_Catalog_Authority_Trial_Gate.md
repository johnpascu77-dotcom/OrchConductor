# OrchConductor Phase 5F — Runtime Catalog Authority Trial Gate

## Purpose

Phase 5F introduces a second, stricter, non-default build flag for future runtime-catalog authority experiments:

```cmake
ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL
```

This flag does not change MIDI behavior by itself. It creates an explicit internal gate for future code paths that may compare or trial runtime-catalog authority while preserving the default hardcoded-authority behavior.

## Relationship to Runtime JSON

The existing runtime JSON infrastructure flag remains:

```cmake
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
```

Runtime JSON enables loading, parsing, probing, and auditing the embedded factory JSON preset catalog.

The new authority trial flag is stricter. It is only valid when runtime JSON is also enabled.

## Valid Configurations

### Default production-safe configuration

```powershell
cmake -S . -B build
```

Expected behavior:

- Runtime JSON presets are disabled.
- Runtime catalog authority trial is disabled.
- Existing hardcoded behavior remains authoritative.

### Runtime JSON probe configuration

```powershell
cmake -S . -B build-runtime-json-on -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

Expected behavior:

- Runtime JSON presets are enabled.
- Runtime catalog authority trial remains disabled unless explicitly requested.
- Runtime catalog probing and audit paths can run.

### Internal authority trial configuration

```powershell
cmake -S . -B build-runtime-authority-trial -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON -DORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL=ON
```

Expected behavior:

- Runtime JSON presets are enabled.
- Runtime catalog authority trial compile definition is enabled.
- Future internal-only authority trial code may be compiled behind this gate.

## Invalid Configuration

The following configuration is intentionally rejected:

```powershell
cmake -S . -B build-invalid-authority-trial -DORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL=ON
```

Expected result:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL requires ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

## Safety Guarantees

Phase 5F is intentionally a gate-only phase.

It does not:

- change preset authority
- change MIDI generation
- change editor population
- route production behavior through runtime JSON
- alter factory preset values

It only establishes the explicit internal compile-time boundary required before runtime-catalog authority can be trialed safely in later phases.

## Validation

Recommended validation:

```powershell
cmake -S . -B build-phase5F-off
cmake --build build-phase5F-off --target OrchConductorProcessorJsonProbeCheck --config Debug
.\build-phase5F-off\Tools\Debug\OrchConductorProcessorJsonProbeCheck.exe
```

```powershell
cmake -S . -B build-phase5F-json-on -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
cmake --build build-phase5F-json-on --target OrchConductorProcessorJsonProbeCheck --config Debug
.\build-phase5F-json-on\Tools\Debug\OrchConductorProcessorJsonProbeCheck.exe
```

```powershell
cmake -S . -B build-phase5F-authority-trial -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON -DORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL=ON
cmake --build build-phase5F-authority-trial --target OrchConductorProcessorJsonProbeCheck --config Debug
.\build-phase5F-authority-trial\Tools\Debug\OrchConductorProcessorJsonProbeCheck.exe
```

And the intentionally invalid configuration:

```powershell
cmake -S . -B build-phase5F-invalid -DORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL=ON
```

This should fail during CMake configuration.
