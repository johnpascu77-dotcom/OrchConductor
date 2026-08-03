#include "OrchConductorEditor.h"

OrchConductorAudioProcessorEditor::OrchConductorAudioProcessorEditor (OrchConductorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (620, 560);

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

    buildLabel.setText ("Build: Phase 1A.3", juce::dontSendNotification);
    buildLabel.setJustificationType (juce::Justification::centred);
    buildLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (140, 160, 180));
    buildLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (buildLabel);

    presetLabel.setText ("Preset", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    presetLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (presetLabel);

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
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
    presetBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colour::fromRGB (95, 200, 245));
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

    sendButton.setButtonText ("Send Preset CCs");
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

    tableTitleLabel.setText ("Selected Preset Output", juce::dontSendNotification);
    tableTitleLabel.setJustificationType (juce::Justification::centred);
    tableTitleLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (245, 245, 245));
    tableTitleLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (tableTitleLabel);

    tableHeaderLabel.setText ("Instrument                 CC      Value", juce::dontSendNotification);
    tableHeaderLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
    tableHeaderLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    addAndMakeVisible (tableHeaderLabel);

    tableRowsLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 230, 235));
    tableRowsLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
    tableRowsLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (tableRowsLabel);

    ccMapLabel.setText (
        "Phase 1A.3 CC Map: CC20 Violin I | CC21 Violin II | CC22 Viola | CC23 Cello | CC24 Double Bass",
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

    auto tableArea = juce::Rectangle<float> (60.0f, 285.0f, 500.0f, 150.0f);
    g.setColour (juce::Colour::fromRGB (24, 32, 42));
    g.fillRoundedRectangle (tableArea, 6.0f);

    g.setColour (juce::Colour::fromRGB (55, 75, 90));
    g.drawRoundedRectangle (tableArea, 6.0f, 1.0f);
}

void OrchConductorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (32);

    titleLabel.setBounds (area.removeFromTop (42));
    subtitleLabel.setBounds (area.removeFromTop (24));
    buildLabel.setBounds (area.removeFromTop (22));

    area.removeFromTop (18);

    auto row = area.removeFromTop (42);
    presetLabel.setBounds (row.removeFromLeft (135));
    presetBox.setBounds (row.removeFromLeft (280));

    area.removeFromTop (12);

    sendOnChangeToggle.setBounds (area.removeFromTop (28).withSizeKeepingCentre (260, 24));

    area.removeFromTop (16);

    auto buttonRow = area.removeFromTop (44);
    sendButton.setBounds (buttonRow.removeFromLeft (270).withSizeKeepingCentre (220, 38));
    allOffButton.setBounds (buttonRow.removeFromLeft (270).withSizeKeepingCentre (190, 38));

    area.removeFromTop (24);

    tableTitleLabel.setBounds (area.removeFromTop (28));
    tableHeaderLabel.setBounds (area.removeFromTop (28).reduced (64, 0));
    tableRowsLabel.setBounds (area.removeFromTop (110).reduced (64, 0));

    area.removeFromTop (16);

    ccMapLabel.setBounds (area.removeFromTop (32));

    area.removeFromTop (10);

    statusLabel.setBounds (area.removeFromTop (32));
}

void OrchConductorAudioProcessorEditor::updateStatus()
{
    const juce::String autoSendText = audioProcessor.getSendOnPresetChange() ? " | Auto-send: On" : " | Auto-send: Off";
    statusLabel.setText ("Selected preset: " + audioProcessor.getPresetName() + autoSendText, juce::dontSendNotification);
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


