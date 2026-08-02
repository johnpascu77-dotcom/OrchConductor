cmake_minimum_required(VERSION 3.22)

project(OrchConductor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(C:/JUCE JUCE)

juce_add_plugin(OrchConductor
    COMPANY_NAME "AudioDev"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Jpas
    PLUGIN_CODE Ocnd
    FORMATS VST3
    PRODUCT_NAME "OrchConductor"
)

target_sources(OrchConductor
    PRIVATE
        Source/OrchConductorProcessor.cpp
        Source/OrchConductorEditor.cpp
)

target_compile_definitions(OrchConductor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OrchConductor
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
