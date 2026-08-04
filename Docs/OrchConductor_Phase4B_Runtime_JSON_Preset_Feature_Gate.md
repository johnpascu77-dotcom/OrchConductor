# OrchConductor Phase 4B - Runtime JSON Preset Feature Gate

## Status

Phase 4B adds a build-time feature gate for future runtime JSON-backed preset work.

The feature gate is disabled by default.

Phase 4B does not change runtime behavior.

## Modified Files

```text
CMakeLists.txt
```

## Added Files

```text
Docs/OrchConductor_Phase4B_Runtime_JSON_Preset_Feature_Gate.md
```

## Feature Gate

The CMake option is:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
```

Default:

```text
OFF
```

When disabled, CMake reports:

```text
OrchConductor runtime JSON presets: disabled
```

When enabled, CMake reports:

```text
OrchConductor runtime JSON presets: ENABLED
```

When enabled, this compile definition is added:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=1
```

## Purpose

The feature gate provides a controlled switch for later runtime JSON preset integration.

It allows future phases to wrap experimental runtime JSON behavior behind:

```cpp
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
```

or:

```cpp
#if defined(ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS)
```

## Current Behavior

Phase 4B does not use the compile definition yet.

Therefore, both configurations should behave identically at runtime:

```text
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
```

## Safety Rules

Phase 4B does not:

```text
Load JSON at runtime
Modify processor behavior
Modify editor behavior
Modify MIDI output
Modify preset switching
Embed JSON resources
Read JSON from disk in the plugin
Replace hardcoded factory presets
Change UI behavior
Change host-facing behavior
```

Existing hardcoded factory preset behavior remains authoritative.

## Build - Default OFF

Configure with default settings:

```powershell
cmake -S . -B build
```

Build:

```powershell
cmake --build build --config Debug
```

Build developer verification target:

```powershell
cmake --build build --config Debug --target OrchConductorPresetLibraryCheck
```

Expected CMake configure message:

```text
OrchConductor runtime JSON presets: disabled
```

## Build - Explicit OFF

```powershell
cmake -S . -B build-runtime-json-off -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=OFF
cmake --build build-runtime-json-off --config Debug
cmake --build build-runtime-json-off --config Debug --target OrchConductorPresetLibraryCheck
```

Expected CMake configure message:

```text
OrchConductor runtime JSON presets: disabled
```

## Build - Explicit ON

```powershell
cmake -S . -B build-runtime-json-on -DORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON
cmake --build build-runtime-json-on --config Debug
cmake --build build-runtime-json-on --config Debug --target OrchConductorPresetLibraryCheck
```

Expected CMake configure message:

```text
OrchConductor runtime JSON presets: ENABLED
```

## Validation

The JSON validation script should still pass:

```powershell
.\Scripts\Validate-OrchConductorLibrary.ps1
```

Expected result:

```text
[PASS] Validation completed successfully with zero failures.
```

The developer preset library check should still pass:

```powershell
Get-ChildItem .\build -Recurse -Filter OrchConductorPresetLibraryCheck.exe
```

Then run the located executable.

Expected final lines:

```text
[PASS] Passive JSON loader verification completed successfully.
[PASS] Factory JSON parity verification completed successfully.
```

## Runtime Boundary

Phase 4B only introduces the switch.

It does not connect the switch to plugin behavior.

Future runtime integration must remain fallback-safe and should continue to preserve hardcoded behavior unless the feature gate is enabled and all validation succeeds.

## Recommended Next Phase

Recommended next phase:

```text
Phase 4C: Embedded Factory JSON Resource Planning or Skeleton
```

A conservative Phase 4C should either:

```text
Document the embedded-resource strategy
```

or:

```text
Add a disabled/non-runtime embedded resource skeleton
```

Runtime JSON loading should still not be activated until a later phase with explicit parity checks and fallback handling.

## Completion Criteria

Phase 4B is complete when:

```text
CMake option exists.
Default value is OFF.
ON configuration defines ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=1.
OFF configuration does not enable runtime JSON behavior.
Default build succeeds.
Explicit OFF build succeeds.
Explicit ON build succeeds.
Developer verification target succeeds.
Runtime processor/editor behavior remains unchanged.
The phase is committed and tagged.
```

Recommended tag:

```text
phase-4B-runtime-json-preset-feature-gate
```
