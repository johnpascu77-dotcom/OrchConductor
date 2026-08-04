\# OrchConductor Phase 4C - Embedded Factory JSON Resource Plan



\## Status



Phase 4C is a planning phase.



No runtime code is changed in Phase 4C.



No build files are changed in Phase 4C.



Phase 4C defines the preferred strategy for embedding the factory JSON preset library into the plugin binary in a later phase.



\## Background



Phase 3 introduced a passive JSON preset library infrastructure:



```text

Formal JSON schema

Factory JSON library

Validation script

Passive C++ data model

Passive JSON loader

Developer verification target

Factory parity verification

```



Phase 4A documented the overall runtime JSON integration plan.



Phase 4B added the build-time feature gate:



```text

ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS

```



The gate defaults to:



```text

OFF

```



When enabled, it defines:



```text

ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS=1

```



Phase 4C decides how factory JSON data should eventually be made available to the plugin runtime.



\## Primary Recommendation



Factory preset JSON should be embedded into the plugin binary.



The factory preset JSON should not be required from disk during normal plugin operation.



Preferred source file:



```text

Examples/orchconductor\_library\_v1.example.json

```



Preferred runtime source model:



```text

Embedded binary/resource data

```



Preferred normal plugin behavior:



```text

No runtime dependency on a JSON file path

No installer dependency for factory preset JSON

No host-dependent file lookup for factory presets

```



\## Why Embedded JSON Is Preferred



Embedding the factory JSON into the binary provides:



```text

Host safety

No missing-file failure mode for factory presets

No dependence on working directory

No dependence on user permissions

No dependence on installer resource paths

No disk I/O during plugin construction

No ambiguity between plugin version and factory data version

Factory data versioned with the plugin binary

Simpler support and diagnostics

Offline reliability

```



This is especially important for plugin environments, where hosts may have different:



```text

Working directories

Sandbox behavior

File access restrictions

Plugin scanning behavior

Startup timing

Logging visibility

```



\## Disk-Loaded JSON Policy



Disk-loaded JSON should not be used for normal factory preset operation.



Disk-loaded JSON may be useful later for:



```text

Developer iteration

External library experiments

User-editable libraries

Third-party expansion libraries

Diagnostic tools

```



However, those are separate from the factory preset source.



If disk loading is added later, it should be:



```text

Explicitly enabled

Non-default for factory presets

Fallback-safe

Clearly separated from embedded factory data

Not required for plugin startup

Not required for preset switching

```



\## Recommended Embedded Resource Strategy



Use the JUCE/CMake binary data mechanism if available in the project.



The expected implementation direction is:



```text

1\. Add the factory JSON file as binary data.

2\. Generate a BinaryData symbol for the JSON content.

3\. Expose a small internal helper that returns the embedded JSON as a string\_view-like or pointer/size pair.

4\. Parse the embedded JSON only when ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS is enabled.

5\. Validate the parsed model before any runtime use.

6\. Fall back to hardcoded presets on any failure.

```



The exact generated symbol name should be verified during implementation.



A likely resource name may resemble:



```text

orchconductor\_library\_v1\_example\_json

```



But Phase 4C does not lock this name.



The implementation phase must confirm the actual generated BinaryData symbol.



\## Resource Naming Principles



The embedded resource should be named predictably.



Recommended naming concepts:



```text

Factory JSON library

Schema version v1

OrchConductor-specific

Stable enough for diagnostics

```



Recommended logical identifier:



```text

orchconductor\_factory\_library\_v1\_json

```



Possible generated binary-data identifier:



```text

orchconductor\_library\_v1\_example\_json

```



The implementation should avoid exposing generated resource names across broad runtime code.



Instead, use a small wrapper/helper.



Example conceptual API:



```cpp

namespace orchconductor

{

&#x20;   struct EmbeddedFactoryJson

&#x20;   {

&#x20;       const char\* data = nullptr;

&#x20;       int size = 0;



&#x20;       bool isValid() const noexcept

&#x20;       {

&#x20;           return data != nullptr \&\& size > 0;

&#x20;       }

&#x20;   };



&#x20;   EmbeddedFactoryJson getEmbeddedFactoryJson() noexcept;

}

```



This is conceptual only.



Phase 4C does not add this code.



\## Compile Gate Relationship



Embedded factory JSON support should remain connected to the Phase 4B gate:



```text

ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS

```



Recommended policy:



```text

When OFF:

&#x20; Runtime code does not attempt to parse or use embedded JSON.

&#x20; Existing hardcoded preset behavior remains authoritative.



When ON:

&#x20; Runtime code may attempt to access embedded JSON.

&#x20; Runtime code may parse embedded JSON.

&#x20; Runtime code must validate loaded JSON before use.

&#x20; Runtime code must fall back to hardcoded presets on failure.

```



Adding the embedded resource to the build does not necessarily mean it is used at runtime.



It is acceptable for a future phase to add the embedded resource while still not activating runtime JSON use.



\## Runtime Access Boundary



The embedded JSON should be accessed through a narrow boundary.



Recommended internal boundary:



```text

Source/OrchConductorEmbeddedFactoryJson.h

Source/OrchConductorEmbeddedFactoryJson.cpp

```



Possible responsibilities:



```text

Expose pointer/size access to embedded JSON data

Hide BinaryData generated symbol details

Avoid parsing JSON directly

Avoid owning preset state

Avoid modifying processor/editor behavior by itself

```



This boundary should not:



```text

Switch presets

Emit MIDI

Modify UI

Decide fallback behavior globally

Perform schema parity checks by itself

```



The embedded resource accessor should be a passive utility.



\## Validation Boundary



Embedded JSON must not be trusted merely because it is compiled into the binary.



Before runtime use, embedded JSON must pass the same expectations already established in Phase 3:



```text

Valid JSON syntax

Recognized schema identity

Supported schemaVersion

Expected pluginTarget

Required sections/domains present

Correct preset counts

Unique preset IDs

Sequential factory IDs where required

Section/domain consistency

CC range validation

Value range validation

Reserved CC49 exclusion

Known factory entry checks

Special partial-value checks

Final model isValid() check

```



Known special cases that must remain preserved:



```text

strings.low\_strings       CC52 = 64

solo.english\_horn\_lament  CC54 = 64

woodwinds.all\_off         no values

brass.all\_off             no values

percussion.all\_off        no values

strings.all\_off           no values

manual.sections           no values

```



\## Fallback Requirements



Hardcoded presets remain the fallback authority.



If embedded JSON access, parsing, or validation fails, the plugin must:



```text

Use existing hardcoded factory presets

Open normally in the host

Avoid user-facing startup interruption

Avoid throwing exceptions across plugin boundaries

Avoid changing MIDI behavior

Avoid changing UI behavior

Avoid leaving partial JSON state active

```



Failure cases that must fall back safely include:



```text

Binary data symbol missing or inaccessible

Embedded data size is zero

Embedded data is not valid UTF-8 or expected text

JSON syntax error

Unsupported schemaVersion

Wrong pluginTarget

Missing required preset section

Invalid CC value

Reserved CC49 usage

Duplicate preset IDs

Incorrect preset counts

Known special-case mismatch

Model isValid() failure

```



\## Threading and Realtime Safety



Embedded JSON parsing and validation must not occur on the audio thread.



Rules:



```text

No JSON parsing on the audio thread

No heavy validation on the audio thread

No allocation-heavy setup on the audio thread

No locks added to the realtime MIDI path for JSON access

No file I/O for factory JSON runtime behavior

```



Recommended timing:



```text

Plugin construction or non-realtime initialization path

```



Only validated, prepared data should ever be visible to preset-selection or MIDI-generation paths.



\## UI Requirements



Embedding JSON must not change UI behavior by itself.



Any later runtime use of embedded JSON must preserve:



```text

Preset names

Preset ordering

Sections

Combi choices

Default selections

Existing labels

Existing user workflow

```



UI behavior should remain unchanged until a later phase explicitly opts into JSON-backed data and proves parity.



\## MIDI Requirements



Embedding JSON must not change MIDI behavior by itself.



Any later runtime use of embedded JSON must preserve:



```text

Same CC numbers

Same CC values

Same all-off behavior

Same manual section behavior

Same special partial values

No CC49 emission from presets

No new MIDI messages on load failure

No different output caused by the feature gate when JSON validation fails

```



\## Testing Requirements for Future Implementation



When embedded JSON support is implemented, test at minimum:



```text

Default configuration with runtime JSON gate OFF

Explicit OFF configuration

Explicit ON configuration

Plugin build

Developer verification target build

JSON validation script

Passive JSON loader check

Factory parity check

Plugin opens in supported host

Preset UI opens

Preset selection works

MIDI output matches hardcoded baseline

Fallback path works if embedded JSON is intentionally corrupted in a test branch

```



The ON configuration should prove that enabling the gate does not cause runtime regression.



\## Recommended Phase 4D



Recommended next implementation phase:



```text

Phase 4D: Add Embedded Factory JSON Resource Skeleton

```



Suggested Phase 4D scope:



```text

Add the factory JSON file to binary data/resource generation.

Add a passive embedded JSON accessor wrapper if needed.

Do not parse JSON in the plugin runtime yet.

Do not modify processor/editor behavior.

Do not switch preset source.

Document the generated resource symbol and accessor boundary.

Build with ORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS OFF and ON.

```



Recommended Phase 4D modified files may include:



```text

CMakeLists.txt

Source/OrchConductorEmbeddedFactoryJson.h

Source/OrchConductorEmbeddedFactoryJson.cpp

Docs/OrchConductor\_Phase4D\_Embedded\_Factory\_JSON\_Resource\_Skeleton.md

```



Phase 4D should still avoid activating runtime JSON-backed preset behavior.



\## Longer-Term Sequence



Recommended later sequence:



```text

Phase 4D: Add embedded resource skeleton.

Phase 4E: Add passive embedded JSON loader check target.

Phase 4F: Add runtime construction-time load attempt behind feature gate.

Phase 4G: Add runtime parity comparison against hardcoded presets.

Phase 4H: Allow JSON-backed runtime source only after successful validation and parity.

Phase 4I: Host/UI/MIDI regression testing.

Phase 4J: Decide whether hardcoded duplication can be reduced.

```



\## Completion Criteria for Phase 4C



Phase 4C is complete when:



```text

Embedded factory JSON strategy is documented.

Factory JSON source recommendation is documented.

Disk-loaded JSON policy is documented.

Compile gate relationship is documented.

Runtime access boundary is documented.

Validation and fallback requirements are documented.

No source files are modified.

No build files are modified.

The document is committed and tagged.

```



Recommended tag:



```text

phase-4C-embedded-factory-json-resource-plan

```



