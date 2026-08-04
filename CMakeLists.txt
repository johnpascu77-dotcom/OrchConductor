cmake_minimum_required(VERSION 3.22)

project(OrchConductor VERSION 0.1.0)
option(ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS "Enable experimental runtime JSON-backed preset infrastructure" OFF)

if(ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS)
    message(STATUS "OrchConductor runtime JSON presets: ENABLED")
    add_compile_definitions(ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS=1)
else()
    message(STATUS "OrchConductor runtime JSON presets: disabled")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

target_compile_definitions(OrchConductor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
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

