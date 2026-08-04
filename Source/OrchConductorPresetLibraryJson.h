#pragma once

#include "OrchConductorPresetLibrary.h"

namespace orchconductor
{

struct PresetLibraryJsonLoadResult
{
    PresetLibraryDefinition library;
    juce::String errorMessage;

    bool wasOk() const noexcept
    {
        return errorMessage.isEmpty();
    }
};

class PresetLibraryJsonLoader
{
public:
    static PresetLibraryJsonLoadResult fromJsonText(const juce::String& jsonText);
    static PresetLibraryJsonLoadResult fromJsonFile(const juce::File& file);
};

} // namespace orchconductor
