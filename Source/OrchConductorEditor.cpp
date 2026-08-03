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

    void addWoodwindsPresetItems (juce::ComboBox& box)
    {
        box.addItem ("All Off (data only)", 1);
        box.addItem ("Flutes Only", 2);
        box.addItem ("Oboes Only", 3);
        box.addItem ("Clarinets Only", 4);
        box.addItem ("Bassoons Only", 5);
        box.addItem ("Full Woodwinds", 6);
    }

    void addBrassPresetItems (juce::ComboBox& box)
    {
        box.addItem ("All Off (data only)", 1);
        box.addItem ("Horns Only", 2);
        box.addItem ("Trumpets Only", 3);
        box.addItem ("Trombones Only", 4);
        box.addItem ("Tuba Only", 5);
        box.addItem ("Full Brass", 6);
    }

    void addPercussionPresetItems (juce::ComboBox& box)
    {
        box.addItem ("All Off (data only)", 1);
        box.addItem ("Timpani Only (data only)", 2);
        box.addItem ("Cymbals Only (data only)", 3);
        box.addItem ("Snare Only (data only)", 4);
        box.addItem ("Bass Drum Only (data only)", 5);
        box.addItem ("Full Percussion (data only)", 6);
    }

    void addStringsPresetItems (juce::ComboBox& box)
    {
        box.addItem ("All Off", 1);
        box.addItem ("Violin I Only", 2);
        box.addItem ("Violin II Only", 3);
        box.addItem ("Violins Only", 4);
        box.addItem ("Violas Only", 5);
        box.addItem ("Cellos Only", 6);
        box.addItem ("Basses Only", 7);
        box.addItem ("Upper Strings", 8);
        box.addItem ("Low Strings", 9);
        box.addItem ("String Quartet", 10);
        box.addItem ("Viola + Cello", 11);
        box.addItem ("Cello + Bass", 12);
        box.addItem ("Full Strings", 13);
        box.addItem ("Tutti", 14);
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

    buildLabel.setText ("Build: Phase 1E", juce::dontSendNotification);
    buildLabel.setJustificationType (juce::Justification::centred);
    buildLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (140, 160, 180));
    buildLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (buildLabel);

    combiPresetLabel.setText ("Combi Preset", juce::dontSendNotification);
    styleLabel (combiPresetLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (combiPresetLabel);

    combiPresetBox.addItem ("Manual Sections", 1);
    combiPresetBox.setSelectedId (audioProcessor.getCombiPresetId() + 1, juce::dontSendNotification);
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

    addWoodwindsPresetItems (woodwindsPresetBox);
    woodwindsPresetBox.setSelectedId (audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::woodwinds) + 1, juce::dontSendNotification);
    styleComboBox (woodwindsPresetBox, true);
    addAndMakeVisible (woodwindsPresetBox);

    addBrassPresetItems (brassPresetBox);
    brassPresetBox.setSelectedId (audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::brass) + 1, juce::dontSendNotification);
    styleComboBox (brassPresetBox, true);
    addAndMakeVisible (brassPresetBox);

    addPercussionPresetItems (percussionPresetBox);
    percussionPresetBox.setSelectedId (audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::percussion) + 1, juce::dontSendNotification);
    styleComboBox (percussionPresetBox, true);
    addAndMakeVisible (percussionPresetBox);

    addStringsPresetItems (presetBox);
    presetBox.setSelectedId (audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::strings) + 1, juce::dontSendNotification);
    styleComboBox (presetBox, true);
    addAndMakeVisible (presetBox);

    woodwindsPresetBox.onChange = [this]
    {
        const int selected = woodwindsPresetBox.getSelectedId() - 1;
        audioProcessor.setSectionPresetId (OrchConductorAudioProcessor::Section::woodwinds, selected);
        updateWoodwindsOutputTable();
        updateStatus();
    };

    brassPresetBox.onChange = [this]
    {
        const int selected = brassPresetBox.getSelectedId() - 1;
        audioProcessor.setSectionPresetId (OrchConductorAudioProcessor::Section::brass, selected);
        updateBrassOutputTable();
        updateStatus();
    };

    percussionPresetBox.onChange = [this]
    {
        const int selected = percussionPresetBox.getSelectedId() - 1;
        audioProcessor.setSectionPresetId (OrchConductorAudioProcessor::Section::percussion, selected);
        updateStatus();
    };

    presetBox.onChange = [this]
    {
        const int selected = presetBox.getSelectedId() - 1;

        if (selected >= 0 && selected <= static_cast<int> (OrchConductorAudioProcessor::Preset::tutti))
            audioProcessor.setSectionPresetId (OrchConductorAudioProcessor::Section::strings, selected);

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
        statusLabel.setText ("Requested send: " + audioProcessor.getPresetName() + " | MIDI-capable: Woodwinds/Brass", juce::dontSendNotification);
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

    woodwindsPanelLabel.setText ("Woodwinds", juce::dontSendNotification);
    woodwindsPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (woodwindsPanelLabel, juce::Colour::fromRGB (245, 245, 245), 14.0f, juce::Font::bold);
    addAndMakeVisible (woodwindsPanelLabel);

    woodwindsTableHeaderLabel.setText ("Instrument                 CC      Value", juce::dontSendNotification);
    woodwindsTableHeaderLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
    woodwindsTableHeaderLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::bold));
    addAndMakeVisible (woodwindsTableHeaderLabel);

    woodwindsTableRowsLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 230, 235));
    woodwindsTableRowsLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    woodwindsTableRowsLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (woodwindsTableRowsLabel);

    brassPanelLabel.setText ("Brass", juce::dontSendNotification);
    brassPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (brassPanelLabel, juce::Colour::fromRGB (245, 245, 245), 14.0f, juce::Font::bold);
    addAndMakeVisible (brassPanelLabel);

    brassTableHeaderLabel.setText ("Instrument                 CC      Value", juce::dontSendNotification);
    brassTableHeaderLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
    brassTableHeaderLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::bold));
    addAndMakeVisible (brassTableHeaderLabel);

    brassTableRowsLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 230, 235));
    brassTableRowsLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    brassTableRowsLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (brassTableRowsLabel);

    percussionPanelLabel.setText ("Percussion\nPreset shell active\nMIDI planned", juce::dontSendNotification);
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
        "Phase 1E Active MIDI Map: Strings CC20-CC24 | Woodwinds CC30-CC33 | Brass CC40-CC43 | Percussion data-only",
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
    updateWoodwindsOutputTable();
    updateBrassOutputTable();
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

    woodwindsPanelLabel.setBounds (48, 416, 424, 22);
    woodwindsTableHeaderLabel.setBounds (88, 442, 340, 22);
    woodwindsTableRowsLabel.setBounds (88, 466, 340, 64);

    brassPanelLabel.setBounds (508, 416, 424, 22);
    brassTableHeaderLabel.setBounds (548, 442, 340, 22);
    brassTableRowsLabel.setBounds (548, 466, 340, 64);

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

    statusLabel.setText (
        "Selected strings preset: " + audioProcessor.getPresetName()
        + " | MIDI-capable: Woodwinds/Brass | Perc data-only"
        + autoSendText,
        juce::dontSendNotification);
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

void OrchConductorAudioProcessorEditor::updateWoodwindsOutputTable()
{
    juce::String rows;

    for (int i = 0; i < OrchConductorAudioProcessor::getNumWoodwindsOutputRows(); ++i)
    {
        const auto row = audioProcessor.getWoodwindsOutputRow (i);

        rows << row.instrumentName.paddedRight (' ', 24)
             << juce::String (row.ccNumber).paddedRight (' ', 8)
             << juce::String (row.value)
             << "\n";
    }

    woodwindsTableRowsLabel.setText (rows, juce::dontSendNotification);
}




void OrchConductorAudioProcessorEditor::updateBrassOutputTable()
{
    juce::String rows;

    for (int i = 0; i < OrchConductorAudioProcessor::getNumBrassOutputRows(); ++i)
    {
        const auto row = audioProcessor.getBrassOutputRow (i);

        rows << row.instrumentName.paddedRight (' ', 24)
             << juce::String (row.ccNumber).paddedRight (' ', 8)
             << juce::String (row.value)
             << "\n";
    }

    brassTableRowsLabel.setText (rows, juce::dontSendNotification);
}








