#pragma once

#include <JuceHeader.h>
#include "OrchConductorProcessor.h"

class OrchConductorAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                           public juce::Timer
{
public:
    explicit OrchConductorAudioProcessorEditor (OrchConductorAudioProcessor&);
    ~OrchConductorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
private:
    OrchConductorAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label buildLabel;

    juce::Label combiPresetLabel;
    juce::Label authorityModeLabel;
    juce::Label narrativeScanLabel;
    juce::Label narrativeScanStatusLabel;
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
    juce::Label percussionTableHeaderLabel;
    juce::Label percussionTableRowsLabel;
    juce::Label stringsPanelLabel;
    juce::Label tableHeaderLabel;
    juce::Label tableRowsLabel;
    juce::Label narrativeMetadataLabel;
    juce::Label narrativeMetadataValueLabel;
    juce::Label ccMapLabel;
    juce::Label statusLabel;

    juce::ComboBox combiPresetBox;
    juce::ComboBox authorityModeBox;
    juce::ComboBox narrativeLaneBox;
    juce::Slider narrativePositionSlider;
    juce::ComboBox woodwindsPresetBox;
    juce::ComboBox brassPresetBox;
    juce::ComboBox percussionPresetBox;
    juce::ComboBox presetBox;

    juce::ToggleButton sendOnChangeToggle;
    juce::Label inputPassthroughLabel;
    juce::ComboBox inputPassthroughBox;
    juce::TextButton sendButton;
    juce::TextButton allOffButton;
    juce::TextButton midiMapButton;

    juce::Label userCombiNameLabel;
    juce::TextEditor userCombiNameEditor;

    // Harp (CC49) / Piano (CC55): a live Manual Sections control surface
    // (they have no section-preset dropdown), also captured by "Save Current
    // Sections as Combi". -1 = Off. Named userCombi* for historical reasons;
    // they drive audioProcessor.setManual{Harp,Piano}Value.
    juce::Label userCombiHarpLabel;
    juce::Slider userCombiHarpSlider;
    juce::Label userCombiPianoLabel;
    juce::Slider userCombiPianoSlider;

    juce::TextButton saveUserCombiButton { "Save Current Sections as Combi" };
    juce::TextButton deleteUserCombiButton { "Delete User Combi" };
    juce::TextButton exportUserCombisButton { "Export User Combis JSON" };
    juce::TextButton importUserCombisButton { "Import User Combis JSON" };

    juce::TextButton exportNarrativeLibraryButton { "Export Lane Library JSON" };
    juce::TextButton importNarrativeLibraryButton { "Import Lane Library JSON" };
    juce::Label narrativeLibrarySourceLabel;

    juce::String lastActionText { "Last action: None" };
    int sendRequestCount { 0 };
    std::unique_ptr<juce::FileChooser> userCombiExportChooser;
    std::unique_ptr<juce::FileChooser> userCombiImportChooser;
    std::unique_ptr<juce::FileChooser> narrativeLibraryExportChooser;
    std::unique_ptr<juce::FileChooser> narrativeLibraryImportChooser;
    void updateStatus();
    void updateNarrativeMetadataDisplay();
    void updateNarrativeScanControls();
    void rebuildNarrativeLaneItems();
    void updateOutputTable();
    void updateWoodwindsOutputTable();
    void updateBrassOutputTable();
    void updatePercussionOutputTable();

    juce::String buildMidiMapText() const;
    void showMidiMap();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchConductorAudioProcessorEditor)
};






