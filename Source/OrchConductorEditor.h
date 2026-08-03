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

    juce::Label combiPresetLabel;
    juce::Label sectionPresetsLabel;
    juce::Label woodwindsPresetLabel;
    juce::Label brassPresetLabel;
    juce::Label percussionPresetLabel;
    juce::Label stringsPresetLabel;

    juce::Label tableTitleLabel;
    juce::Label woodwindsPanelLabel;
    juce::Label woodwindsTableHeaderLabel;
    juce::Label woodwindsTableRowsLabel;
    juce::Label brassPanelLabel;
    juce::Label brassTableHeaderLabel;
    juce::Label brassTableRowsLabel;
    juce::Label percussionPanelLabel;
    juce::Label stringsPanelLabel;
    juce::Label tableHeaderLabel;
    juce::Label tableRowsLabel;
    juce::Label ccMapLabel;
    juce::Label statusLabel;

    juce::ComboBox combiPresetBox;
    juce::ComboBox woodwindsPresetBox;
    juce::ComboBox brassPresetBox;
    juce::ComboBox percussionPresetBox;
    juce::ComboBox presetBox;

    juce::ToggleButton sendOnChangeToggle;
    juce::TextButton sendButton;
    juce::TextButton allOffButton;

    void updateStatus();
    void updateOutputTable();
    void updateWoodwindsOutputTable();
    void updateBrassOutputTable();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchConductorAudioProcessorEditor)
};




