#pragma once

#include <JuceHeader.h>

#include "OrchConductorPresetLibrary.h"
#include "OrchConductorRuntimePresetSource.h"

// Read-only, non-authoritative runtime preset catalog adapter.
//
// Phase 4L exposes only passive factory-shape metadata.
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

    int getSectionCount() const noexcept;
    int getCombiPresetCount() const noexcept;
    bool hasExpectedFactoryShape() const noexcept;

private:
    OrchConductorRuntimePresetCatalog(bool ready,
                                      bool fallbackRequired,
                                      juce::String diagnosticMessage,
                                      orchconductor::PresetLibraryDefinition library);

    bool ready_ = false;
    bool fallbackRequired_ = true;
    juce::String diagnosticMessage_;
    orchconductor::PresetLibraryDefinition library_;
};
