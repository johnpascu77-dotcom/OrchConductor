#include "OrchConductorRuntimePresetCatalog.h"

OrchConductorRuntimePresetCatalog OrchConductorRuntimePresetCatalog::createFallbackCatalog()
{
    return OrchConductorRuntimePresetCatalog(
        true,
        true,
        "Runtime preset catalog using hardcoded fallback data.");
}

OrchConductorRuntimePresetCatalog OrchConductorRuntimePresetCatalog::createFromRuntimeSource(
    const orchconductor::RuntimePresetSourceResult& source)
{
    const auto sourceDiagnostic = source.diagnosticMessage;

    if (source.wasLoaded() && ! source.requiresHardcodedFallback())
    {
        return OrchConductorRuntimePresetCatalog(
            true,
            false,
            sourceDiagnostic.isNotEmpty()
                ? "Runtime preset catalog ready from runtime source: " + sourceDiagnostic
                : "Runtime preset catalog ready from runtime source.");
    }

    return OrchConductorRuntimePresetCatalog(
        true,
        true,
        sourceDiagnostic.isNotEmpty()
            ? "Runtime preset catalog using fallback because runtime source is unavailable: " + sourceDiagnostic
            : "Runtime preset catalog using fallback because runtime source is unavailable.");
}

bool OrchConductorRuntimePresetCatalog::isReady() const noexcept
{
    return ready_;
}

bool OrchConductorRuntimePresetCatalog::requiresFallback() const noexcept
{
    return fallbackRequired_;
}

juce::String OrchConductorRuntimePresetCatalog::getDiagnosticMessage() const
{
    return diagnosticMessage_;
}

OrchConductorRuntimePresetCatalog::OrchConductorRuntimePresetCatalog(
    bool ready,
    bool fallbackRequired,
    juce::String diagnosticMessage)
    : ready_(ready),
      fallbackRequired_(fallbackRequired),
      diagnosticMessage_(std::move(diagnosticMessage))
{
}
