#pragma once

#include <JuceHeader.h>
#include "OrchConductorProcessor.h"

class OrchConductorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit OrchConductorAudioProcessorEditor (OrchConductorAudioProcessor&);
    ~OrchConductorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OrchConductorAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label buildLabel;
    juce::Label presetLabel;
    juce::Label ccMapLabel;
    juce::Label statusLabel;

    juce::ComboBox presetBox;
    juce::ToggleButton sendOnChangeToggle;
    juce::TextButton sendButton;
    juce::TextButton allOffButton;

    void updateStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchConductorAudioProcessorEditor)
};
