#include "OrchConductorRuntimePresetSource.h"

#include "OrchConductorEmbeddedFactoryJson.h"
#include "OrchConductorPresetLibraryJson.h"

namespace orchconductor
{

RuntimePresetSourceResult RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled()
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    const auto embeddedJson = getEmbeddedFactoryJson();

    if (! embeddedJson.isValid())
    {
        RuntimePresetSourceResult result;
        result.diagnosticMessage = "Embedded factory JSON resource is unavailable.";
        return result;
    }

    const auto jsonText = juce::String::fromUTF8(embeddedJson.data, embeddedJson.size);

    if (jsonText.isEmpty())
    {
        RuntimePresetSourceResult result;
        result.diagnosticMessage = "Embedded factory JSON resource converted to empty text.";
        return result;
    }

    auto loadResult = PresetLibraryJsonLoader::fromJsonText(jsonText);

    if (! loadResult.wasOk())
    {
        RuntimePresetSourceResult result;
        result.diagnosticMessage = "Embedded factory JSON parse failed: " + loadResult.errorMessage;
        return result;
    }

    if (! loadResult.library.isValid())
    {
        RuntimePresetSourceResult result;
        result.diagnosticMessage = "Embedded factory JSON library failed final isValid() check.";
        return result;
    }

    RuntimePresetSourceResult result;
    result.status = RuntimePresetSourceStatus::loaded;
    result.library = std::move(loadResult.library);
    result.diagnosticMessage = "Embedded factory JSON loaded successfully.";
    return result;
#else
    RuntimePresetSourceResult result;
    result.diagnosticMessage = "Runtime JSON presets are disabled by ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS.";
    return result;
#endif
}

RuntimePresetSourceResult RuntimePresetSource::loadFromJsonFileIfEnabled(const juce::File& file)
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    if (! file.existsAsFile())
    {
        RuntimePresetSourceResult result;
        result.diagnosticMessage = "Narrative library file does not exist: " + file.getFullPathName();
        return result;
    }

    auto loadResult = PresetLibraryJsonLoader::fromJsonFile(file);

    if (! loadResult.wasOk())
    {
        RuntimePresetSourceResult result;
        result.diagnosticMessage = "Narrative library JSON parse failed: " + loadResult.errorMessage;
        return result;
    }

    if (! loadResult.library.isValid())
    {
        RuntimePresetSourceResult result;
        result.diagnosticMessage = "Narrative library JSON failed final isValid() check.";
        return result;
    }

    RuntimePresetSourceResult result;
    result.status = RuntimePresetSourceStatus::loaded;
    result.library = std::move(loadResult.library);
    result.diagnosticMessage = "Narrative library JSON loaded from " + file.getFileName();
    return result;
#else
    juce::ignoreUnused(file);
    RuntimePresetSourceResult result;
    result.diagnosticMessage = "Runtime JSON presets are disabled by ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS.";
    return result;
#endif
}

} // namespace orchconductor
