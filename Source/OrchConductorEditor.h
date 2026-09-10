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

    // View switch: Conductor controls / Combi Grid / Narrative Lane Maker.
    enum class View { conductor = 0, grid, lane };
    juce::TextButton conductorViewButton { "Conductor" };
    juce::TextButton gridViewButton { "Combi Grid" };
    juce::TextButton laneViewButton { "Narrative Lane" };
    std::unique_ptr<juce::Component> instrumentGridView;   // an InstrumentGridComponent (.cpp-private)
    std::unique_ptr<juce::Component> laneMakerView;        // a LaneMakerComponent (.cpp-private)
    void showView (View view);

    // The Conductor controls live in a scroll viewport so the window can be
    // resized well below the content height. The view-switch buttons, the
    // grid, and the footer status lines stay pinned outside it. conductorContent
    // forwards wheel/trackpad scroll to the viewport even over child controls
    // (it is a WheelForwardingComponent, defined in the .cpp).
    juce::Viewport conductorViewport;
    std::unique_ptr<juce::Component> conductorContent;

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

    // Live "Gate Response" panel: broadcasts CC 106 (mode) / CC 107 (amount)
    // that every OrchGate set to "Follow Conductor Response" obeys - a single
    // control for the whole rig's invert / threshold / participation
    // randomisation. Overrides a narrative lane point's own gate-response
    // values while enabled.
    juce::Label gateResponseLabel;
    juce::ToggleButton gateResponseEnableButton { "Broadcast Gate Response" };
    juce::Label gateResponseAmountLabel;
    juce::Slider gateResponseAmountSlider;
    juce::Label gateResponseModeLabel;
    juce::Slider gateResponseModeSlider;
    juce::TextButton gateResponseShuffleButton { "Shuffle" };
    juce::Label gateResponseStatusLabel;

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






