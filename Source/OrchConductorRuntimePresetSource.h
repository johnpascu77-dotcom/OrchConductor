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

    // Same contract as loadEmbeddedFactoryJsonIfEnabled(), but reads an
    // external orchconductor_library_v1 JSON file instead of the compiled-in
    // resource. Used by the editor's "Import Lane Library JSON" action so the
    // narrative-lane library can grow without a rebuild. Returns
    // status == unavailable (with a diagnostic) if the feature is disabled,
    // the file is missing, or it fails to parse/validate.
    static RuntimePresetSourceResult loadFromJsonFileIfEnabled(const juce::File& file);
};

} // namespace orchconductor
