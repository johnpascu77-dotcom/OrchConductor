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

    void addCombiPresetItems (juce::ComboBox& box)
    {
        box.addItem ("Manual Sections", 1);

        box.addItem ("[Utility] All Off", 2);
        box.addItem ("[Utility] Full Orchestra", 3);
        box.addItem ("[Utility] Full Orchestra No Percussion", 4);
        box.addItem ("[Utility] Chamber Orchestra", 5);
        box.addItem ("[Utility] Full Strings", 6);
        box.addItem ("[Utility] Full Woodwinds", 7);
        box.addItem ("[Utility] Full Brass", 8);
        box.addItem ("[Utility] Full Winds", 9);
        box.addItem ("[Utility] High Orchestra", 10);
        box.addItem ("[Utility] Low Orchestra", 11);
        box.addItem ("[Utility] Middle Orchestra", 12);

        box.addItem ("[Romantic] Warm Strings + Horns", 13);
        box.addItem ("[Romantic] Oboe + Strings", 14);
        box.addItem ("[Romantic] Flute + Violins", 15);
        box.addItem ("[Romantic] Bassoon + Celli", 16);
        box.addItem ("[Romantic] Horn Choir + Strings", 17);

        box.addItem ("[Cinematic] Heroic Brass + Strings", 18);
        box.addItem ("[Cinematic] Dark Trailer Bed", 19);
        box.addItem ("[Cinematic] High Winds Shimmer", 20);
        box.addItem ("[Cinematic] Epic Low Pulse", 21);

        box.addItem ("[Herrmann] Low Reeds", 22);
        box.addItem ("[Herrmann] Horn Knives", 23);
        box.addItem ("[Herrmann] Psycho Strings", 24);
        box.addItem ("[Herrmann] Suspense Winds", 25);

        box.addItem ("[Modernist] Pointillist Winds", 26);
        box.addItem ("[Modernist] Sparse Extremes", 27);
        box.addItem ("[Shimmer] Silver Shimmer", 28);
        box.addItem ("[Solo] English Horn Lament", 29);
    }
    void addWoodwindsPresetItems (juce::ComboBox& box)
    {
        box.addItem ("All Off", 1);

        box.addItem ("Piccolo Only", 2);

        box.addItem ("Flutes", 3);
        box.addItem ("Flute 1 Only", 4);
        box.addItem ("Flute 2 Only", 5);

        box.addItem ("Oboes", 6);
        box.addItem ("Oboe 1 Only", 7);
        box.addItem ("Oboe 2 Only", 8);
        box.addItem ("English Horn Only", 9);

        box.addItem ("Clarinets", 10);
        box.addItem ("Clarinet 1 Only", 11);
        box.addItem ("Clarinet 2 Only", 12);
        box.addItem ("Bass Clarinet Only", 13);

        box.addItem ("Bassoons", 14);
        box.addItem ("Bassoon 1 Only", 15);
        box.addItem ("Bassoon 2 Only", 16);
        box.addItem ("Contrabassoon Only", 17);

        box.addItem ("High Woodwinds", 18);
        box.addItem ("Low Woodwinds", 19);
        box.addItem ("Full Woodwinds", 20);
    }

    void addBrassPresetItems (juce::ComboBox& box)
    {
        box.addItem ("All Off", 1);

        box.addItem ("Horns", 2);
        box.addItem ("Horn 1 Only", 3);
        box.addItem ("Horn 2 Only", 4);
        box.addItem ("Horn 3 Only", 5);
        box.addItem ("Horn 4 Only", 6);

        box.addItem ("Trumpets", 7);
        box.addItem ("Trumpet 1 Only", 8);
        box.addItem ("Trumpet 2 Only", 9);
        box.addItem ("Trumpet 3 Only", 10);

        box.addItem ("Trombones", 11);
        box.addItem ("Trombone 1 Only", 12);
        box.addItem ("Trombone 2 Only", 13);
        box.addItem ("Bass Trombone Only", 14);

        box.addItem ("Tuba Only", 15);

        box.addItem ("Low Brass", 16);
        box.addItem ("Full Brass", 17);
    }

    void addPercussionPresetItems (juce::ComboBox& box)
    {
        box.addItem ("All Off", 1);
        box.addItem ("Timpani Only", 2);
        box.addItem ("Glockenspiel Only", 3);
        box.addItem ("Xylophone Only", 4);
        box.addItem ("Marimba Only", 5);
        box.addItem ("Vibraphone Only", 6);
        box.addItem ("Tubular Bells Only", 7);
        box.addItem ("Mallets", 8);
        box.addItem ("Full Melodic Percussion", 9);
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
    setSize (980, 560);

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

    buildLabel.setText ("Build: Phase 6C-3", juce::dontSendNotification);
    buildLabel.setJustificationType (juce::Justification::centred);
    buildLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (140, 160, 180));
    buildLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (buildLabel);

    combiPresetLabel.setText ("Combi Preset", juce::dontSendNotification);
    styleLabel (combiPresetLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (combiPresetLabel);

    addCombiPresetItems (combiPresetBox);
    combiPresetBox.setSelectedId (audioProcessor.getCombiPresetId() + 1, juce::dontSendNotification);
    styleComboBox (combiPresetBox, true);
    addAndMakeVisible (combiPresetBox);

    combiPresetBox.onChange = [this]
    {
        const int selected = combiPresetBox.getSelectedId() - 1;
        audioProcessor.setCombiPresetIdFromUI (selected);
        updateStatus();
    };

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
        audioProcessor.setSectionPresetIdFromUI (OrchConductorAudioProcessor::Section::woodwinds, selected);
        updateWoodwindsOutputTable();
        updateStatus();
    };

    brassPresetBox.onChange = [this]
    {
        const int selected = brassPresetBox.getSelectedId() - 1;
        audioProcessor.setSectionPresetIdFromUI (OrchConductorAudioProcessor::Section::brass, selected);
        updateBrassOutputTable();
        updateStatus();
    };

    percussionPresetBox.onChange = [this]
    {
        const int selected = percussionPresetBox.getSelectedId() - 1;
        audioProcessor.setSectionPresetIdFromUI (OrchConductorAudioProcessor::Section::percussion, selected);
        updatePercussionOutputTable();
    updateStatus();

    // Phase 2A: keep UI synced when host restores plugin state after editor creation.
    startTimerHz (10);
};

    presetBox.onChange = [this]
    {
        const int selected = presetBox.getSelectedId() - 1;

        if (selected >= 0 && selected <= static_cast<int> (OrchConductorAudioProcessor::Preset::tutti))
            audioProcessor.setSectionPresetIdFromUI (OrchConductorAudioProcessor::Section::strings, selected);

        updateOutputTable();

        if (audioProcessor.getSendOnPresetChange())
        {
            ++sendRequestCount;
            lastActionText = "Last action: Auto-send requested: " + audioProcessor.getPresetName();
        }

        updateStatus();
    };

    sendOnChangeToggle.setButtonText ("Send on Preset Change");
    sendOnChangeToggle.setToggleState (audioProcessor.getSendOnPresetChange(), juce::dontSendNotification);
    sendOnChangeToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible (sendOnChangeToggle);

    sendOnChangeToggle.onClick = [this]
    {
        audioProcessor.setSendOnPresetChangeFromUI (sendOnChangeToggle.getToggleState());
        updateStatus();
    };

    sendButton.setButtonText ("Send Current Presets");
    sendButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (45, 75, 95));
    sendButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (sendButton);

    sendButton.onClick = [this]
    {
        audioProcessor.requestSendPreset();
        ++sendRequestCount;

        lastActionText = "Last action: Send Current Presets requested: "
                         + (audioProcessor.isCombiModeActive() ? audioProcessor.getCombiPresetName()
                                                                : audioProcessor.getPresetName());

        updateStatus();
    };

    allOffButton.setButtonText ("Send All Off");
    allOffButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (95, 45, 45));
    allOffButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (allOffButton);

    allOffButton.onClick = [this]
    {
        audioProcessor.requestSendAllOff();
        ++sendRequestCount;
        lastActionText = "Last action: Send All Off requested";
        updateStatus();
    };

    midiMapButton.setButtonText ("Show MIDI Map");
    midiMapButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (45, 60, 80));
    midiMapButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (midiMapButton);

    midiMapButton.onClick = [this]
    {
        showMidiMap();
    };

    tableTitleLabel.setText ("Selected Output", juce::dontSendNotification);
    tableTitleLabel.setJustificationType (juce::Justification::centred);
    tableTitleLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (245, 245, 245));
    tableTitleLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    // Phase 1G cleanup: selected-output table moved to MIDI Map popup.

    woodwindsPanelLabel.setText ("Woodwinds", juce::dontSendNotification);
    woodwindsPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (woodwindsPanelLabel, juce::Colour::fromRGB (245, 245, 245), 14.0f, juce::Font::bold);
    // Permanent Woodwinds table hidden; map is available via Show MIDI Map.

    woodwindsTableHeaderLabel.setText ("Instrument                 CC      Value   Players", juce::dontSendNotification);
    woodwindsTableHeaderLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
    woodwindsTableHeaderLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    // Hidden in main UI.

    woodwindsTableRowsLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 230, 235));
    woodwindsTableRowsLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    woodwindsTableRowsLabel.setJustificationType (juce::Justification::topLeft);
    // Hidden in main UI.

    brassPanelLabel.setText ("Brass", juce::dontSendNotification);
    brassPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (brassPanelLabel, juce::Colour::fromRGB (245, 245, 245), 14.0f, juce::Font::bold);
    // Permanent Brass table hidden; map is available via Show MIDI Map.

    brassTableHeaderLabel.setText ("Instrument                 CC      Value   Players", juce::dontSendNotification);
    brassTableHeaderLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
    brassTableHeaderLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    // Hidden in main UI.

    brassTableRowsLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 230, 235));
    brassTableRowsLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    brassTableRowsLabel.setJustificationType (juce::Justification::topLeft);
    // Hidden in main UI.

    percussionPanelLabel.setText ("Percussion", juce::dontSendNotification);
    percussionPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (percussionPanelLabel, juce::Colour::fromRGB (245, 245, 245), 14.0f, juce::Font::bold);
    // Permanent Percussion table hidden; map is available via Show MIDI Map.

    percussionTableHeaderLabel.setText ("Instrument                 CC      Value   Players", juce::dontSendNotification);
    percussionTableHeaderLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
    percussionTableHeaderLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    // Hidden in main UI.

    percussionTableRowsLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 230, 235));
    percussionTableRowsLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    percussionTableRowsLabel.setJustificationType (juce::Justification::topLeft);
    // Hidden in main UI.

    stringsPanelLabel.setText ("Strings", juce::dontSendNotification);
    stringsPanelLabel.setJustificationType (juce::Justification::centred);
    styleLabel (stringsPanelLabel, juce::Colour::fromRGB (245, 245, 245), 14.0f, juce::Font::bold);
    // Permanent Strings table hidden; map is available via Show MIDI Map.

    tableHeaderLabel.setText ("Instrument                 CC      Value   Players", juce::dontSendNotification);
    tableHeaderLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
    tableHeaderLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    // Hidden in main UI.

    tableRowsLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 230, 235));
    tableRowsLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    tableRowsLabel.setJustificationType (juce::Justification::topLeft);
    // Hidden in main UI.

    ccMapLabel.setText (
        "Phase 6C-3: Full-score CC20-CC54 | Bidirectional automatable controls | CC49 reserved for Harp | UI feedback active",
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
    updatePercussionOutputTable();
    updateStatus();

    // Phase 2A: keep UI synced when host restores plugin state after editor creation.
    startTimerHz (10);
}

OrchConductorAudioProcessorEditor::~OrchConductorAudioProcessorEditor()
{
    stopTimer();
}

void OrchConductorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (18, 24, 31));

    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour::fromRGB (95, 200, 245));
    g.drawRoundedRectangle (bounds, 8.0f, 2.0f);

    const auto panelColour = juce::Colour::fromRGB (24, 32, 42);
    const auto outlineColour = juce::Colour::fromRGB (55, 75, 90);

    const juce::Rectangle<float> futureArea (48.0f, 390.0f, 884.0f, 80.0f);

    g.setColour (panelColour);
    g.fillRoundedRectangle (futureArea, 6.0f);

    g.setColour (outlineColour);
    g.drawRoundedRectangle (futureArea, 6.0f, 1.0f);

    g.setColour (juce::Colour::fromRGB (120, 140, 155));
    g.setFont (juce::FontOptions (13.0f, juce::Font::plain));
    g.drawText ("Phase 6C-3: Bidirectional UI-host automation sync active | Runtime JSON diagnostics passive | UI feedback active",
                futureArea.toNearestInt().reduced (16, 8),
                juce::Justification::centred);
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

    midiMapButton.setBounds (390, 374, 200, 34);

    ccMapLabel.setBounds (48, 484, 884, 24);
    statusLabel.setBounds (48, 510, 884, 28);
}


void OrchConductorAudioProcessorEditor::timerCallback()
{
    // Phase 2A: Bitwig may restore plugin state after the editor has already been created.
    // Keep visible controls synced to processor state without firing onChange callbacks.

    const int combiId = audioProcessor.getCombiPresetId() + 1;
    if (combiPresetBox.getSelectedId() != combiId)
        combiPresetBox.setSelectedId (combiId, juce::dontSendNotification);

    const int woodwindsId = audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::woodwinds) + 1;
    if (woodwindsPresetBox.getSelectedId() != woodwindsId)
        woodwindsPresetBox.setSelectedId (woodwindsId, juce::dontSendNotification);

    const int brassId = audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::brass) + 1;
    if (brassPresetBox.getSelectedId() != brassId)
        brassPresetBox.setSelectedId (brassId, juce::dontSendNotification);

    const int percussionId = audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::percussion) + 1;
    if (percussionPresetBox.getSelectedId() != percussionId)
        percussionPresetBox.setSelectedId (percussionId, juce::dontSendNotification);

    const int stringsId = audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::strings) + 1;
    if (presetBox.getSelectedId() != stringsId)
        presetBox.setSelectedId (stringsId, juce::dontSendNotification);

    const bool sendOnChange = audioProcessor.getSendOnPresetChange();
    if (sendOnChangeToggle.getToggleState() != sendOnChange)
        sendOnChangeToggle.setToggleState (sendOnChange, juce::dontSendNotification);

    updateStatus();
}

void OrchConductorAudioProcessorEditor::updateStatus()
{
    const juce::String autoSendText = audioProcessor.getSendOnPresetChange() ? " | Auto-send: On" : " | Auto-send: Off";
    const juce::String activePlayersText = " | Active players: " + juce::String (audioProcessor.getTotalActivePlayers());
    const juce::String sendFeedbackText = " | " + lastActionText + " | Send requests: " + juce::String (sendRequestCount);

    if (audioProcessor.isCombiModeActive())
    {
        statusLabel.setText (
            "Combi active: " + audioProcessor.getCombiPresetName()
            + activePlayersText
            + " | Harp reserved"
            + autoSendText
            + sendFeedbackText,
            juce::dontSendNotification);

        return;
    }

    statusLabel.setText (
        "Manual Sections | Strings: " + audioProcessor.getPresetName()
        + activePlayersText
        + " | Harp reserved"
        + autoSendText
        + sendFeedbackText,
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
             << juce::String (row.value).paddedRight (' ', 8)
             << juce::String (row.activePlayers) << "/" << juce::String (row.maxPlayers)
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
             << juce::String (row.value).paddedRight (' ', 8)
             << juce::String (row.activePlayers) << "/" << juce::String (row.maxPlayers)
             << "\n";
    }

    woodwindsTableRowsLabel.setText (rows, juce::dontSendNotification);
}




juce::String OrchConductorAudioProcessorEditor::buildMidiMapText() const
{
    juce::String text;

    text << "Full-score MIDI Gate Map\n\n";

    text << "Woodwinds\n";
    for (int i = 0; i < OrchConductorAudioProcessor::getNumWoodwindsOutputRows(); ++i)
    {
        const auto row = audioProcessor.getWoodwindsOutputRow (i);
        text << "CC" << juce::String (row.ccNumber).paddedRight (' ', 4)
             << " " << row.instrumentName
             << " | Value " << juce::String (row.value)
             << " | Players " << juce::String (row.activePlayers) << "/" << juce::String (row.maxPlayers)
             << "\n";
    }

    text << "\nBrass\n";
    for (int i = 0; i < OrchConductorAudioProcessor::getNumBrassOutputRows(); ++i)
    {
        const auto row = audioProcessor.getBrassOutputRow (i);
        text << "CC" << juce::String (row.ccNumber).paddedRight (' ', 4)
             << " " << row.instrumentName
             << " | Value " << juce::String (row.value)
             << " | Players " << juce::String (row.activePlayers) << "/" << juce::String (row.maxPlayers)
             << "\n";
    }

    text << "\nMelodic Percussion\n";
    for (int i = 0; i < OrchConductorAudioProcessor::getNumPercussionOutputRows(); ++i)
    {
        const auto row = audioProcessor.getPercussionOutputRow (i);
        text << "CC" << juce::String (row.ccNumber).paddedRight (' ', 4)
             << " " << row.instrumentName
             << " | Value " << juce::String (row.value)
             << " | Players " << juce::String (row.activePlayers) << "/" << juce::String (row.maxPlayers)
             << "\n";
    }

    text << "\nReserved\n";
    text << "CC49   Harp\n";

    text << "\nStrings\n";
    for (int i = 0; i < OrchConductorAudioProcessor::getNumOutputRows(); ++i)
    {
        const auto row = audioProcessor.getOutputRow (i);
        text << "CC" << juce::String (row.ccNumber).paddedRight (' ', 4)
             << " " << row.instrumentName
             << " | Value " << juce::String (row.value)
             << " | Players " << juce::String (row.activePlayers) << "/" << juce::String (row.maxPlayers)
             << "\n";
    }

    text << "\nUse these CC numbers as the assigned CC Gate values in each OrchGate instance.";

    return text;
}

void OrchConductorAudioProcessorEditor::showMidiMap()
{
    juce::AlertWindow::showMessageBoxAsync (
        juce::AlertWindow::InfoIcon,
        "OrchConductor MIDI Map",
        buildMidiMapText(),
        "OK");
}

void OrchConductorAudioProcessorEditor::updateBrassOutputTable()
{
    juce::String rows;

    for (int i = 0; i < OrchConductorAudioProcessor::getNumBrassOutputRows(); ++i)
    {
        const auto row = audioProcessor.getBrassOutputRow (i);

        rows << row.instrumentName.paddedRight (' ', 24)
             << juce::String (row.ccNumber).paddedRight (' ', 8)
             << juce::String (row.value).paddedRight (' ', 8)
             << juce::String (row.activePlayers) << "/" << juce::String (row.maxPlayers)
             << "\n";
    }

    brassTableRowsLabel.setText (rows, juce::dontSendNotification);
}

void OrchConductorAudioProcessorEditor::updatePercussionOutputTable()
{
    juce::String rows;

    for (int i = 0; i < OrchConductorAudioProcessor::getNumPercussionOutputRows(); ++i)
    {
        const auto row = audioProcessor.getPercussionOutputRow (i);

        rows << row.instrumentName.paddedRight (' ', 24)
             << juce::String (row.ccNumber).paddedRight (' ', 8)
             << juce::String (row.value).paddedRight (' ', 8)
             << juce::String (row.activePlayers) << "/" << juce::String (row.maxPlayers)
             << "\n";
    }

    percussionTableRowsLabel.setText (rows, juce::dontSendNotification);
}










