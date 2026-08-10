#pragma once

#include <JuceHeader.h>

#include "OrchConductorPresetLibrary.h"
#include "OrchConductorRuntimePresetSource.h"

// Read-only, non-authoritative runtime preset catalog adapter.
//
// Phase 4M exposes passive factory-shape metadata and label accessors.
// It does not expose preset values.
// It does not participate in editor population.
// It does not participate in MIDI generation.
// It does not parse JSON directly.
struct OrchConductorRuntimePresetValueView
{
    int ccNumber = -1;
    int value = -1;
    bool isValid = false;
};

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
    int getSectionPresetCount(const juce::String& sectionId) const noexcept;
    int getCombiPresetCount() const noexcept;
    bool hasExpectedFactoryShape() const noexcept;

    juce::String getSectionPresetLabel(const juce::String& sectionId, int presetIndex) const;
    juce::String getCombiPresetLabel(int presetIndex) const;
    bool getCombiPresetNarrativeMetadata(int presetIndex,
                                         orchconductor::NarrativeMetadata& metadata) const;

    int getSectionPresetValueCount(const juce::String& sectionId, int presetIndex) const noexcept;
    int getCombiPresetValueCount(int presetIndex) const noexcept;

    OrchConductorRuntimePresetValueView getSectionPresetValue(const juce::String& sectionId,
                                                              int presetIndex,
                                                              int valueIndex) const noexcept;
    OrchConductorRuntimePresetValueView getCombiPresetValue(int presetIndex,
                                                            int valueIndex) const noexcept;

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

