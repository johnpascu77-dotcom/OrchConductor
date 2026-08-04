cmake_minimum_required(VERSION 3.22)

project(OrchConductor VERSION 0.1.0)

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

target_sources(OrchConductor
    PRIVATE
        Source/OrchConductorProcessor.cpp
        Source/OrchConductorEditor.cpp
        Source/OrchConductorPresetLibrary.cpp
        Source/OrchConductorPresetLibrary.h
        Source/OrchConductorPresetLibraryJson.cpp
        Source/OrchConductorPresetLibraryJson.h
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
