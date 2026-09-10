cmake_minimum_required(VERSION 3.22)

project(OrchConductor VERSION 0.1.0)

# Force synchronous PDB writes under parallel (/MP-style) compilation - without
# this, building a target with many translation units (JUCE's amalgamated
# module .cpp files included) intermittently fails with C1041 "cannot open
# program database" when two CL.EXE instances write the same .pdb at once.
if (MSVC)
    add_compile_options(/FS)
endif()

# ON by default since 2026-09-10: this is the build the rig actually uses.
# It loads the embedded orchconductor_library_v1 JSON, which is the ONLY
# source of the 6 narrative-scan lanes (the hardcoded fallback catalog has
# none). The JSON stays NON-authoritative for CC values - the hardcoded
# combi/section tables remain the source of truth for MIDI output unless
# ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL is also ON (it is not).
option(ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS "Enable runtime JSON-backed preset infrastructure (narrative-scan lanes)" ON)
option(ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL "Enable internal non-default runtime catalog authority trial path" OFF)

if(ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL AND NOT ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS)
    message(FATAL_ERROR "ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL requires ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=ON")
endif()

if(ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS)
    message(STATUS "OrchConductor runtime JSON presets: ENABLED")
    add_compile_definitions(ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=1)
else()
    message(STATUS "OrchConductor runtime JSON presets: disabled")
endif()

if(ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL)
    message(STATUS "OrchConductor runtime catalog authority trial: ENABLED")
    add_compile_definitions(ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL=1)
else()
    message(STATUS "OrchConductor runtime catalog authority trial: disabled")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build-timestamp label (see cmake/GenerateOrchConductorBuildInfo.cmake): a
# plain custom target with no OUTPUT/DEPENDS tracking, so it reruns and
# rewrites the header on every single build, not just every configure or
# only when a source file happens to change.
set(OrchConductorGeneratedDir "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${OrchConductorGeneratedDir}")

add_custom_target(OrchConductorGenerateBuildInfo ALL
    COMMAND ${CMAKE_COMMAND}
        "-DOUTPUT_FILE=${OrchConductorGeneratedDir}/OrchConductorBuildInfo.h"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateOrchConductorBuildInfo.cmake"
    BYPRODUCTS "${OrchConductorGeneratedDir}/OrchConductorBuildInfo.h"
    COMMENT "Stamping OrchConductor build timestamp"
)

# Path to your JUCE folder.
add_subdirectory("C:/JUCE/JUCE" JUCE)

juce_add_plugin(OrchConductor
    COMPANY_NAME "JohnPascu"
    PLUGIN_MANUFACTURER_CODE Jpas
    PLUGIN_CODE Ocnd

    FORMATS VST3

    PRODUCT_NAME "OrchConductor"
    PLUGIN_NAME "OrchConductor"
    BUNDLE_ID "com.johnpascu.OrchConductor"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_REPLACES_VST2 FALSE

    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(OrchConductor)
# Embedded factory JSON resource data.
juce_add_binary_data(OrchConductorFactoryJsonData
    SOURCES
        Examples/orchconductor_library_v1.example.json
)

# /FS (see top of file) hasn't reliably prevented this small target's
# generated BinaryData*.cpp from colliding with other targets' concurrent
# writes to a shared .pdb. /Z7 embeds debug info directly in each .obj
# instead, sidestepping the shared-PDB contention for this target entirely -
# it's just a data blob, there's nothing meaningful to step through anyway.
if (MSVC)
    target_compile_options(OrchConductorFactoryJsonData PRIVATE /Z7)
endif()

target_sources(OrchConductor
    PRIVATE
        Source/OrchConductorProcessor.cpp
        Source/OrchConductorEditor.cpp
        Source/OrchConductorPresetLibrary.cpp
        Source/OrchConductorPresetLibrary.h
        Source/OrchConductorPresetLibraryJson.cpp
        Source/OrchConductorPresetLibraryJson.h
        Source/OrchConductorEmbeddedFactoryJson.cpp
        Source/OrchConductorEmbeddedFactoryJson.h
        Source/OrchConductorRuntimePresetSource.cpp
        Source/OrchConductorRuntimePresetCatalog.cpp
        Source/OrchConductorRuntimePresetCatalog.h
        Source/OrchConductorRuntimePresetSource.h
        Source/OrchConductorNarrativeScanResolver.cpp
        Source/OrchConductorNarrativeScanResolver.h
)

add_dependencies(OrchConductor OrchConductorGenerateBuildInfo)
target_include_directories(OrchConductor PRIVATE ${OrchConductorGeneratedDir})

target_compile_definitions(OrchConductor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JucePlugin_Name=\"OrchConductor\"
)

target_link_libraries(OrchConductor
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_extra
        OrchConductorFactoryJsonData

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# JUCE 9 VST3 wrapper warning control.
if (TARGET OrchConductor_VST3)
    target_compile_definitions(OrchConductor_VST3
        PRIVATE
            JUCE_VST3_CAN_REPLACE_VST2=0
    )
endif()

# Developer-only verification target for the passive preset-library JSON loader.
add_executable(OrchConductorPresetLibraryCheck
    Tools/OrchConductorPresetLibraryCheck.cpp
    Source/OrchConductorPresetLibrary.cpp
    Source/OrchConductorPresetLibraryJson.cpp
)

target_link_libraries(OrchConductorPresetLibraryCheck
    PRIVATE
        juce::juce_core
)

target_include_directories(OrchConductorPresetLibraryCheck
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
)

target_compile_definitions(OrchConductorPresetLibraryCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Developer-only verification target for the embedded factory JSON accessor.
add_executable(OrchConductorEmbeddedFactoryJsonCheck
    Tools/OrchConductorEmbeddedFactoryJsonCheck.cpp
    Source/OrchConductorPresetLibrary.cpp
    Source/OrchConductorPresetLibraryJson.cpp
    Source/OrchConductorEmbeddedFactoryJson.cpp
)

target_link_libraries(OrchConductorEmbeddedFactoryJsonCheck
    PRIVATE
        juce::juce_core
        OrchConductorFactoryJsonData
)

target_include_directories(OrchConductorEmbeddedFactoryJsonCheck
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
)

target_compile_definitions(OrchConductorEmbeddedFactoryJsonCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Developer-only verification target for the runtime JSON preset source boundary.
add_executable(OrchConductorRuntimePresetSourceCheck
    Tools/OrchConductorRuntimePresetSourceCheck.cpp
    Source/OrchConductorPresetLibrary.cpp
    Source/OrchConductorPresetLibraryJson.cpp
    Source/OrchConductorEmbeddedFactoryJson.cpp
    Source/OrchConductorRuntimePresetSource.cpp
)

target_link_libraries(OrchConductorRuntimePresetSourceCheck
    PRIVATE
        juce::juce_core
        OrchConductorFactoryJsonData
)

target_include_directories(OrchConductorRuntimePresetSourceCheck
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
)

target_compile_definitions(OrchConductorRuntimePresetSourceCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Developer-only parity verification target for the runtime JSON preset source boundary.
add_executable(OrchConductorRuntimePresetSourceParityCheck
    Tools/OrchConductorRuntimePresetSourceParityCheck.cpp
    Source/OrchConductorPresetLibrary.cpp
    Source/OrchConductorPresetLibraryJson.cpp
    Source/OrchConductorEmbeddedFactoryJson.cpp
    Source/OrchConductorRuntimePresetSource.cpp
)

target_link_libraries(OrchConductorRuntimePresetSourceParityCheck
    PRIVATE
        juce::juce_core
        OrchConductorFactoryJsonData
)

target_include_directories(OrchConductorRuntimePresetSourceParityCheck
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
)

target_compile_definitions(OrchConductorRuntimePresetSourceParityCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# Developer-only verification target for the processor construction-time JSON probe.
juce_add_console_app(OrchConductorProcessorJsonProbeCheck
    PRODUCT_NAME "OrchConductorProcessorJsonProbeCheck"
)

juce_generate_juce_header(OrchConductorProcessorJsonProbeCheck)

target_sources(OrchConductorProcessorJsonProbeCheck
    PRIVATE
        Tools/OrchConductorProcessorJsonProbeCheck.cpp
        Source/OrchConductorProcessor.cpp
        Source/OrchConductorEditor.cpp
        Source/OrchConductorPresetLibrary.cpp
        Source/OrchConductorPresetLibraryJson.cpp
        Source/OrchConductorEmbeddedFactoryJson.cpp
        Source/OrchConductorRuntimePresetSource.cpp
        Source/OrchConductorRuntimePresetCatalog.cpp
        Source/OrchConductorNarrativeScanResolver.cpp
)

target_link_libraries(OrchConductorProcessorJsonProbeCheck
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_extra
        OrchConductorFactoryJsonData

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

add_dependencies(OrchConductorProcessorJsonProbeCheck OrchConductorGenerateBuildInfo)

target_include_directories(OrchConductorProcessorJsonProbeCheck
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
        ${OrchConductorGeneratedDir}
)

target_compile_definitions(OrchConductorProcessorJsonProbeCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JucePlugin_Name=\"OrchConductor\"
)

# Developer-only MIDI regression verification target for the processor JSON probe.
juce_add_console_app(OrchConductorProcessorMidiRegressionCheck
    PRODUCT_NAME "OrchConductorProcessorMidiRegressionCheck"
)

juce_generate_juce_header(OrchConductorProcessorMidiRegressionCheck)

target_sources(OrchConductorProcessorMidiRegressionCheck
    PRIVATE
        Tools/OrchConductorProcessorMidiRegressionCheck.cpp
        Source/OrchConductorProcessor.cpp
        Source/OrchConductorEditor.cpp
        Source/OrchConductorPresetLibrary.cpp
        Source/OrchConductorPresetLibraryJson.cpp
        Source/OrchConductorEmbeddedFactoryJson.cpp
        Source/OrchConductorRuntimePresetSource.cpp
        Source/OrchConductorRuntimePresetCatalog.cpp
        Source/OrchConductorNarrativeScanResolver.cpp
)

target_link_libraries(OrchConductorProcessorMidiRegressionCheck
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_extra
        OrchConductorFactoryJsonData

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

add_dependencies(OrchConductorProcessorMidiRegressionCheck OrchConductorGenerateBuildInfo)

target_include_directories(OrchConductorProcessorMidiRegressionCheck
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
        ${OrchConductorGeneratedDir}
)

if (MSVC)
    # /FS alone hasn't reliably prevented C1041 (concurrent PDB writes) for
    # this target in this environment; force serial compilation as a
    # guaranteed fix (slower, but this is a developer-only check tool, not a
    # hot rebuild loop). /MP1 appended last overrides the multi-process
    # compilation this target otherwise inherits.
    target_compile_options(OrchConductorProcessorMidiRegressionCheck PRIVATE /FS /MP1)
endif()

target_compile_definitions(OrchConductorProcessorMidiRegressionCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JucePlugin_Name=\"OrchConductor\"
)

# Developer-only read-only runtime preset catalog adapter verification target.
juce_add_console_app(OrchConductorRuntimePresetCatalogCheck
    PRODUCT_NAME "OrchConductorRuntimePresetCatalogCheck"
)

juce_generate_juce_header(OrchConductorRuntimePresetCatalogCheck)

target_sources(OrchConductorRuntimePresetCatalogCheck
    PRIVATE
        Tools/OrchConductorRuntimePresetCatalogCheck.cpp
        Source/OrchConductorPresetLibrary.cpp
        Source/OrchConductorPresetLibraryJson.cpp
        Source/OrchConductorEmbeddedFactoryJson.cpp
        Source/OrchConductorRuntimePresetSource.cpp
        Source/OrchConductorRuntimePresetCatalog.cpp
)

target_link_libraries(OrchConductorRuntimePresetCatalogCheck
    PRIVATE
        juce::juce_core
        OrchConductorFactoryJsonData

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(OrchConductorRuntimePresetCatalogCheck
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
)

target_compile_definitions(OrchConductorRuntimePresetCatalogCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Developer-only verification target for narrative scan resolver behavior.
juce_add_console_app(OrchConductorNarrativeScanResolverCheck
    PRODUCT_NAME "OrchConductorNarrativeScanResolverCheck"
)

juce_generate_juce_header(OrchConductorNarrativeScanResolverCheck)

target_sources(OrchConductorNarrativeScanResolverCheck
    PRIVATE
        Tools/OrchConductorNarrativeScanResolverCheck.cpp
        Source/OrchConductorPresetLibrary.cpp
        Source/OrchConductorPresetLibraryJson.cpp
        Source/OrchConductorEmbeddedFactoryJson.cpp
        Source/OrchConductorRuntimePresetSource.cpp
        Source/OrchConductorRuntimePresetCatalog.cpp
        Source/OrchConductorNarrativeScanResolver.cpp
)

target_link_libraries(OrchConductorNarrativeScanResolverCheck
    PRIVATE
        juce::juce_core
        OrchConductorFactoryJsonData

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(OrchConductorNarrativeScanResolverCheck
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Tools
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(OrchConductorNarrativeScanResolverCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=1
)


