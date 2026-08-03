#include "OrchConductorEditor.h"

namespace
{
    void styleLabel (juce::Label& label, juce::Colour colour, float size, int style = juce::Font::plain)
    {
        label.setColour (juce::Label::textColourId, colour);
        label.setFont (juce::FontOptions (size, style));
    }

    void styleComboBox (juce::ComboBox& box, bool enabled)
    {
        box.setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
        box.setColour (juce::ComboBox::textColourId, enabled ? juce::Colours::white : juce::Colour::fromRGB (145, 155, 165));
        box.setColour (juce::ComboBox::outlineColourId, enabled ? juce::Colour::fromRGB (95, 200, 245)
                                                                 : juce::Colour::fromRGB (70, 85, 95));
        box.setEnabled (enabled);
    }
}

OrchConductorAudioProcessorEditor::OrchConductorAudioProcessorEditor (OrchConductorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (980, 760);

    titleLabel.setText ("OrchConductor", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Orchestration Preset Sender", juce::dontSendNotification);
    subtitleLabel.setJustificationType (juce::Justification::centred);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    subtitleLabel.setFont (juce::FontOptions (15.0f));
    addAndMakeVisible (subtitleLabel);

    buildLabel.setText ("Build: Phase 1A.4", juce::dontSendNotification);
    buildLabel.setJustificationType (juce::Justification::centred);
    buildLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (140, 160, 180));
    buildLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (buildLabel);

    combiPresetLabel.setText ("Combi Preset", juce::dontSendNotification);
    styleLabel (combiPresetLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (combiPresetLabel);

    combiPresetBox.addItem ("Manual Sections / Coming Soon", 1);
    combiPresetBox.setSelectedId (1, juce::dontSendNotification);
    styleComboBox (combiPresetBox, false);
    addAndMakeVisible (combiPresetBox);

    sectionPresetsLabel.setText ("Section Presets", juce::dontSendNotification);
    sectionPresetsLabel.setJustificationType (juce::Justification::centred);
    styleLabel (sectionPresetsLabel, juce::Colour::fromRGB (245, 245, 245), 15.0f, juce::Font::bold);
    addAndMakeVisible (sectionPresetsLabel);

    woodwindsPresetLabel.setText ("Woodwinds", juce::dontSendNotification);
    styleLabel (woodwindsPresetLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (woodwindsPresetLabel);

    brassPresetLabel.setText ("Brass", juce::dontSendNotification);
    styleLabel (brassPresetLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (brassPresetLabel);

    percussionPresetLabel.setText ("Percussion", juce::dontSendNotification);
    styleLabel (percussionPresetLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (percussionPresetLabel);

    stringsPresetLabel.setText ("Strings", juce::dontSendNotification);
    styleLabel (stringsPresetLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (stringsPresetLabel);

    woodwindsPresetBox.addItem ("Coming Soon", 1);
    woodwindsPresetBox.setSelectedId (1, juce::dontSendNotification);
    styleComboBox (woodwindsPresetBox, false);
    addAndMakeVisible (woodwindsPresetBox);

    brassPresetBox.addItem ("Coming Soon", 1);
    brassPresetBox.setSelectedId (1, juce::dontSendNotification);
    styleComboBox (brassPresetBox, false);
    addAndMakeVisible (brassPresetBox);

    percussionPresetBox.addItem ("Coming Soon", 1);
    percussionPresetBox.setSelectedId (1, juce::dontSendNotification);
    styleComboBox (percussionPresetBox, false);
    addAndMakeVisible (percussionPresetBox);

    presetBox.addItem ("All Off", 1);
    presetBox.addItem ("Violin I Only", 2);
    presetBox.addItem ("Violin II Only", 3);
    presetBox.addItem ("Violins Only", 4);
    presetBox.addItem ("Violas Only", 5);
    presetBox.addItem ("Cellos Only", 6);
    presetBox.addItem ("Basses Only", 7);
    presetBox.addItem ("Upper Strings", 8);
    presetBox.addItem ("Low Strings", 9);
    presetBox.addItem ("String Quartet", 10);
    presetBox.addItem ("Viola + Cello", 11);
    presetBox.addItem ("Cello + Bass", 12);
    presetBox.addItem ("Full Strings", 13);
    presetBox.addItem ("Tutti", 14);
    presetBox.setSelectedId (static_cast<int> (audioProcessor.getPreset()) + 1, juce::dontSendNotification);
    styleComboBox (presetBox, true);
    addAndMakeVisible (presetBox);

    presetBox.onChange = [this]
    {
        const int selected = presetBox.getSelectedId() - 1;

        if (selected >= 0 && selected <= static_cast<int> (OrchConductorAudioProcessor::Preset::tutti))
            audioProcessor.setPreset (static_cast<OrchConductorAudioProcessor::Preset> (selected));

        updateOutputTable();

        if (audioProcessor.getSendOnPresetChange())
            statusLabel.setText ("Auto-send requested: " + audioProcessor.getPresetName(), juce::dontSendNotification);
        else
            updateStatus();
    };

    sendOnChangeToggle.setButtonText ("Send on Preset Change");
    sendOnChangeToggle.setToggleState (audioProcessor.getSendOnPresetChange(), juce::dontSendNotification);
    sendOnChangeToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible (sendOnChangeToggle);

    sendOnChangeToggle.onClick = [this]
    {
        audioProcessor.setSendOnPresetChange (sendOnChangeToggle.getToggleState());
        updateStatus();
    };

    sendButton.setButtonText ("Send Current Presets");
    sendButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (45, 75, 95));
    sendButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (sendButton);

    sendButton.onClick = [this]
    {
        audioProcessor.requestSendPreset();
        statusLabel.setText ("Requested send: " + audioProcessor.getPresetName(), juce::dontSendNotification);
    };

    allOffButton.setButtonText ("Send All Off");
    allOffButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (95, 45, 45));
    allOffButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (allOffButton);

    allOffButton.onClick = [this]
    {
        audioProcessor.requestSendAllOff();
        statusLabel.setText ("Requested send: All Off", juce::dontSendNotification);
    };

    tableTitleLabel.setText ("Selected Output", juce::dontSendNotification);
    tableTitleLabel.setJustificationType (juce::Justification::centred);
    tableTitleLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (245, 245, 245));
    tableTitleLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (tableTitleLabel);

    woodwindsPanelLabel.setText ("Woodwinds\nComing soon", juce::dontSendNotification);
    woodwindsPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (woodwindsPanelLabel, juce::Colour::fromRGB (160, 175, 190), 14.0f, juce::Font::bold);
    addAndMakeVisible (woodwindsPanelLabel);

    brassPanelLabel.setText ("Brass\nComing soon", juce::dontSendNotification);
    brassPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (brassPanelLabel, juce::Colour::fromRGB (160, 175, 190), 14.0f, juce::Font::bold);
    addAndMakeVisible (brassPanelLabel);

    percussionPanelLabel.setText ("Percussion\nComing soon", juce::dontSendNotification);
    percussionPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (percussionPanelLabel, juce::Colour::fromRGB (160, 175, 190), 14.0f, juce::Font::bold);
    addAndMakeVisible (percussionPanelLabel);

    stringsPanelLabel.setText ("Strings", juce::dontSendNotification);
    stringsPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (stringsPanelLabel, juce::Colour::fromRGB (245, 245, 245), 14.0f, juce::Font::bold);
    addAndMakeVisible (stringsPanelLabel);

    tableHeaderLabel.setText ("Instrument                 CC      Value", juce::dontSendNotification);
    tableHeaderLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
    tableHeaderLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::bold));
    addAndMakeVisible (tableHeaderLabel);

    tableRowsLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 230, 235));
    tableRowsLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    tableRowsLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (tableRowsLabel);

    ccMapLabel.setText (
        "Phase 1A.4 Active CC Map: Strings CC20-CC24",
        juce::dontSendNotification);
    ccMapLabel.setJustificationType (juce::Justification::centred);
    ccMapLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (160, 175, 190));
    ccMapLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (ccMapLabel);

    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (245, 195, 90));
    statusLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (statusLabel);

    updateOutputTable();
    updateStatus();
}

OrchConductorAudioProcessorEditor::~OrchConductorAudioProcessorEditor()
{
}

void OrchConductorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (18, 24, 31));

    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour::fromRGB (95, 200, 245));
    g.drawRoundedRectangle (bounds, 8.0f, 2.0f);

    const auto panelColour = juce::Colour::fromRGB (24, 32, 42);
    const auto outlineColour = juce::Colour::fromRGB (55, 75, 90);

    const juce::Rectangle<float> woodwindsArea  (48.0f, 410.0f, 424.0f, 92.0f);
    const juce::Rectangle<float> brassArea      (508.0f, 410.0f, 424.0f, 92.0f);
    const juce::Rectangle<float> percussionArea (48.0f, 518.0f, 424.0f, 150.0f);
    const juce::Rectangle<float> stringsArea    (508.0f, 518.0f, 424.0f, 150.0f);

    for (auto area : { woodwindsArea, brassArea, percussionArea, stringsArea })
    {
        g.setColour (panelColour);
        g.fillRoundedRectangle (area, 6.0f);

        g.setColour (outlineColour);
        g.drawRoundedRectangle (area, 6.0f, 1.0f);
    }
}
void OrchConductorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (48, 28);

    titleLabel.setBounds (area.removeFromTop (38));
    subtitleLabel.setBounds (area.removeFromTop (22));
    buildLabel.setBounds (area.removeFromTop (20));

    area.removeFromTop (12);

    auto combiRow = area.removeFromTop (38);
    combiPresetLabel.setBounds (combiRow.removeFromLeft (150));
    combiPresetBox.setBounds (combiRow.removeFromLeft (650));

    area.removeFromTop (14);

    sectionPresetsLabel.setBounds (area.removeFromTop (26));

    area.removeFromTop (4);

    auto sectionRow1 = area.removeFromTop (38);
    auto leftTop = sectionRow1.removeFromLeft (424);
    sectionRow1.removeFromLeft (36);
    auto rightTop = sectionRow1.removeFromLeft (424);

    woodwindsPresetLabel.setBounds (leftTop.removeFromLeft (110));
    woodwindsPresetBox.setBounds (leftTop);

    brassPresetLabel.setBounds (rightTop.removeFromLeft (110));
    brassPresetBox.setBounds (rightTop);

    area.removeFromTop (8);

    auto sectionRow2 = area.removeFromTop (38);
    auto leftBottom = sectionRow2.removeFromLeft (424);
    sectionRow2.removeFromLeft (36);
    auto rightBottom = sectionRow2.removeFromLeft (424);

    percussionPresetLabel.setBounds (leftBottom.removeFromLeft (110));
    percussionPresetBox.setBounds (leftBottom);

    stringsPresetLabel.setBounds (rightBottom.removeFromLeft (110));
    presetBox.setBounds (rightBottom);

    area.removeFromTop (18);

    auto buttonRow = area.removeFromTop (42);
    sendButton.setBounds (buttonRow.removeFromLeft (280).withSizeKeepingCentre (240, 36));
    allOffButton.setBounds (buttonRow.removeFromLeft (240).withSizeKeepingCentre (190, 36));
    sendOnChangeToggle.setBounds (buttonRow.removeFromLeft (300).withSizeKeepingCentre (260, 24));

    tableTitleLabel.setBounds (48, 374, 884, 28);

    woodwindsPanelLabel.setBounds (48, 410, 424, 92);
    brassPanelLabel.setBounds (508, 410, 424, 92);
    percussionPanelLabel.setBounds (48, 518, 424, 150);

    stringsPanelLabel.setBounds (508, 524, 424, 22);
    tableHeaderLabel.setBounds (548, 552, 340, 22);
    tableRowsLabel.setBounds (548, 576, 340, 88);

    ccMapLabel.setBounds (48, 684, 884, 24);
    statusLabel.setBounds (48, 710, 884, 28);
}
void OrchConductorAudioProcessorEditor::updateStatus()
{
    const juce::String autoSendText = audioProcessor.getSendOnPresetChange() ? " | Auto-send: On" : " | Auto-send: Off";
    statusLabel.setText ("Selected strings preset: " + audioProcessor.getPresetName() + autoSendText, juce::dontSendNotification);
}

void OrchConductorAudioProcessorEditor::updateOutputTable()
{
    juce::String rows;

    for (int i = 0; i < OrchConductorAudioProcessor::getNumOutputRows(); ++i)
    {
        const auto row = audioProcessor.getOutputRow (i);

        rows << row.instrumentName.paddedRight (' ', 24)
             << juce::String (row.ccNumber).paddedRight (' ', 8)
             << juce::String (row.value)
             << "\n";
    }

    tableRowsLabel.setText (rows, juce::dontSendNotification);
}









