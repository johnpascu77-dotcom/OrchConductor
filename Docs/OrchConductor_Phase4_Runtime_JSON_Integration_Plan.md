\# OrchConductor Phase 4A - Runtime JSON Integration Plan



\## Status



Phase 4A is a planning phase.



No runtime code is changed in Phase 4A.



Phase 4A defines how OrchConductor should eventually adopt the Phase 3 JSON factory preset library at runtime while preserving host safety, user expectations, and fallback behavior.



\## Background



Phase 3 completed the JSON library foundation:



```text

JSON factory library

Formal JSON schema

PowerShell validation script

Passive C++ preset data model

Passive JSON loader

Developer loader verification target

Factory parity verification

Phase 3 completion documentation

```



At the end of Phase 3, the plugin runtime still uses existing hardcoded preset behavior.



Phase 4 will define and later implement a controlled transition path from hardcoded factory preset definitions toward JSON-backed factory preset data.



\## Primary Goal



Introduce runtime JSON-backed preset data safely.



The plugin must remain stable if:



```text

JSON is missing

JSON is malformed

JSON schema is unsupported

JSON content fails validation

JSON loading is disabled

Runtime integration is rolled back

```



\## Non-Goals for Phase 4A



Phase 4A does not:



```text

Modify plugin processor behavior

Modify plugin editor behavior

Modify MIDI output behavior

Modify preset switching behavior

Load JSON at runtime

Embed JSON as a binary resource

Add new build options

Change CMake behavior

Change the factory JSON file

Change schemas

Change validation scripts

```



Phase 4A is documentation-only.



\## Runtime Integration Principles



The integration must follow these principles:



```text

1\. Hardcoded factory behavior remains the fallback.

2\. JSON loading must never prevent the plugin from opening.

3\. JSON failure must degrade safely to existing behavior.

4\. Runtime behavior must be parity-checked before activation.

5\. Integration should be behind an explicit gate.

6\. Host-facing behavior must remain stable.

7\. No file-system dependency should be required for normal plugin operation unless explicitly chosen.

8\. Any runtime adoption should be reversible.

```



\## Recommended Integration Strategy



Use a staged approach.



```text

Phase 4A: Plan runtime JSON integration.

Phase 4B: Add compile-time feature gate only.

Phase 4C: Add embedded factory JSON resource or resource-selection decision.

Phase 4D: Add runtime loader path behind disabled-by-default gate.

Phase 4E: Add internal parity comparison against hardcoded factory data.

Phase 4F: Enable JSON-backed factory data only after parity confidence.

Phase 4G: Remove or reduce hardcoded duplication only after long-term stability.

```



\## Feature Gate Strategy



Runtime JSON use should be controlled by an explicit build-time option.



Recommended option name:



```text

ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS

```



Initial default:



```text

OFF

```



Expected behavior:



```text

OFF:

&#x20; Plugin uses existing hardcoded preset behavior.

&#x20; JSON loader code may compile if needed, but it is not used by runtime preset switching.



ON:

&#x20; Plugin may attempt to load factory preset data from the approved JSON source.

&#x20; Failure falls back to hardcoded presets.

```



The gate should be documented and visible in CMake configuration output.



\## Fallback Strategy



Hardcoded presets remain the fallback authority.



If runtime JSON loading fails for any reason, the plugin should:



```text

1\. Report the failure through an internal debug/log mechanism where available.

2\. Avoid user-facing disruption during normal plugin startup.

3\. Continue using hardcoded factory presets.

4\. Preserve existing MIDI output behavior.

5\. Preserve existing UI behavior.

```



Failures that must fall back safely:



```text

Missing JSON data

Invalid JSON syntax

Unsupported schemaVersion

Missing required preset domains

Incorrect preset counts

Out-of-range CC values

Reserved CC49 targeted by a preset

Duplicate preset IDs

Invalid factory IDs

Unexpected section/domain mismatch

```



\## JSON Source Options



There are two practical runtime-source options.



\### Option A - Embedded Factory JSON



Embed the factory JSON into the plugin binary.



Advantages:



```text

No file-system dependency

Stable in plugin hosts

Works offline

Versioned with the binary

Best default for factory presets

```



Disadvantages:



```text

Requires build/resource integration

Changing factory JSON requires rebuild

```



Recommendation:



```text

Preferred for factory presets.

```



\### Option B - Disk-Loaded JSON



Load JSON from a known disk path.



Advantages:



```text

Easier iteration during development

Can support user-editable libraries later

No rebuild needed for content changes

```



Disadvantages:



```text

Host file-system variability

Installer complexity

Missing-file risk

Permission/path issues

Harder support surface

```



Recommendation:



```text

Use only for developer builds or future user-library features.

Do not rely on disk-loaded JSON for factory presets in normal plugin operation.

```



\## Recommended Factory Preset Source



Use embedded JSON for factory presets.



Recommended approach:



```text

Embed Examples/orchconductor\_library\_v1.example.json as a binary/resource asset.

Load from memory at plugin initialization only if the runtime JSON gate is enabled.

Validate the loaded model before exposing it to preset behavior.

Fallback to hardcoded data on any failure.

```



\## Startup Behavior



Startup must be host-safe.



Runtime JSON loading should:



```text

Avoid blocking expensive disk access.

Avoid throwing exceptions across plugin boundaries.

Avoid failing plugin construction.

Avoid requiring network access.

Avoid requiring user interaction.

Avoid changing plugin state if loading fails.

```



Recommended initialization order:



```text

1\. Construct plugin using existing safe defaults.

2\. If runtime JSON gate is enabled, attempt passive load.

3\. Validate loaded library.

4\. Optionally run parity checks in debug/developer builds.

5\. If valid, store loaded library as an internal optional factory source.

6\. If invalid, discard it and use hardcoded presets.

```



\## Validation Requirements Before Runtime Use



Before JSON-backed runtime behavior can be enabled, the loaded library must pass:



```text

Schema identity check

schemaVersion check

pluginTarget check

MIDI range check

Reserved controller policy check

Preset count check

Factory ID sequencing check

Preset ID uniqueness check

Section/domain consistency check

CC range validation

Value range validation

Reserved CC49 exclusion

Known factory entry checks

Special partial-value checks

Final isValid() check

```



These are already represented in Phase 3 validation/check tooling and should be reused or mirrored carefully.



\## Parity Requirements



Before JSON data drives runtime behavior, compare JSON-derived output to current hardcoded output.



Required parity categories:



```text

Preset IDs

Preset display names

Preset ordering

Factory IDs

Section assignments

MIDI CC/value pairs

All-off behavior

Manual sections behavior

Special partial values

Reserved Harp/CC49 behavior

```



Known critical cases:



```text

strings.low\_strings       CC52 = 64

solo.english\_horn\_lament  CC54 = 64

woodwinds.all\_off         no values

brass.all\_off             no values

percussion.all\_off        no values

strings.all\_off           no values

manual.sections           no values

```



\## UI Behavior



Initial runtime JSON integration should not change UI behavior.



The UI should continue to present:



```text

Same preset names

Same preset ordering

Same sections

Same combi choices

Same default selections

```



If a JSON-backed internal source is used, UI labels and ordering must remain identical unless a future version intentionally changes them.



\## MIDI Behavior



MIDI behavior must remain identical.



Required guarantees:



```text

Same CC numbers

Same CC values

Same order if order is behaviorally relevant

No CC49 preset emission

No out-of-range values

No new MIDI messages caused by load failure

```



\## Error Handling



Runtime JSON errors should be non-fatal.



Recommended error policy:



```text

Debug/developer builds:

&#x20; Log detailed load/validation failure.



Release builds:

&#x20; Fail silently or log minimally if a safe logging path exists.

&#x20; Always fall back to hardcoded factory behavior.

```



The plugin should not display modal dialogs from plugin startup or audio/MIDI paths.



\## Threading and Realtime Safety



JSON loading and validation must not occur on the audio thread.



Rules:



```text

No JSON parsing on the audio thread.

No allocation-heavy validation on the audio thread.

No file I/O on the audio thread.

No locks introduced into the realtime MIDI path unless proven safe.

```



Runtime JSON data should be prepared before it can affect preset selection or MIDI generation.



\## Testing Matrix



Before enabling runtime JSON use, test:



```text

Debug build with runtime JSON gate OFF

Debug build with runtime JSON gate ON

Release build with runtime JSON gate OFF

Release build with runtime JSON gate ON

Plugin loads in supported host(s)

Plugin UI opens

Preset selection works

MIDI output matches hardcoded baseline

JSON load failure falls back safely

Malformed JSON falls back safely

Unsupported schemaVersion falls back safely

Missing preset domain falls back safely

Reserved CC49 misuse is rejected

```



\## Rollback Plan



Runtime JSON integration must be easy to disable.



Rollback should be possible by:



```text

Turning ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS OFF

Reverting the runtime integration commit

Falling back automatically on validation failure

Keeping hardcoded presets available until JSON runtime behavior is proven stable

```



\## Recommended Phase 4B



The next implementation phase should be minimal.



Recommended:



```text

Phase 4B: Add Runtime JSON Preset Feature Gate

```



Scope:



```text

Add CMake option ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS default OFF.

Expose a compile definition only when enabled.

Do not change runtime behavior yet.

Document the option.

Build both OFF and ON configurations.

```



Phase 4B should not load JSON at runtime yet.



\## Completion Criteria for Phase 4A



Phase 4A is complete when:



```text

This plan is added to Docs.

No source/runtime files are modified.

No build files are modified.

The document is committed and tagged.

```



Recommended tag:



```text

phase-4A-runtime-json-integration-plan

```



