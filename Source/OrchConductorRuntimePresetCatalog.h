#pragma once

#include <JuceHeader.h>

#include "OrchConductorRuntimePresetSource.h"

// Read-only, non-authoritative runtime preset catalog adapter skeleton.
//
// Phase 4K intentionally exposes only readiness/fallback diagnostics.
// It does not expose preset names.
// It does not expose preset values.
// It does not participate in editor population.
// It does not participate in MIDI generation.
// It does not parse JSON directly.
class OrchConductorRuntimePresetCatalog
{
public:
    static OrchConductorRuntimePresetCatalog createFallbackCatalog();
    static OrchConductorRuntimePresetCatalog createFromRuntimeSource(
        const orchconductor::RuntimePresetSourceResult& source);

    bool isReady() const noexcept;
    bool requiresFallback() const noexcept;
    juce::String getDiagnosticMessage() const;

private:
    OrchConductorRuntimePresetCatalog(bool ready,
                                      bool fallbackRequired,
                                      juce::String diagnosticMessage);

    bool ready_ = false;
    bool fallbackRequired_ = true;
    juce::String diagnosticMessage_;
};
