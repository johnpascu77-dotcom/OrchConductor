#pragma once

#include "OrchConductorPresetLibrary.h"

namespace orchconductor
{

enum class RuntimePresetSourceStatus
{
    unavailable,
    loaded
};

struct RuntimePresetSourceResult
{
    RuntimePresetSourceStatus status = RuntimePresetSourceStatus::unavailable;
    PresetLibraryDefinition library;
    juce::String diagnosticMessage;

    bool wasLoaded() const noexcept
    {
        return status == RuntimePresetSourceStatus::loaded;
    }

    bool requiresHardcodedFallback() const noexcept
    {
        return ! wasLoaded();
    }
};

class RuntimePresetSource
{
public:
    static RuntimePresetSourceResult loadEmbeddedFactoryJsonIfEnabled();
};

} // namespace orchconductor
