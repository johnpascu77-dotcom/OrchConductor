#include "OrchConductorEditor.h"

OrchConductorAudioProcessorEditor::OrchConductorAudioProcessorEditor (OrchConductorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (560, 460);

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

    buildLabel.setText ("Build: Phase 1A.1", juce::dontSendNotification);
    buildLabel.setJustificationType (juce::Justification::centred);
    buildLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (140, 160, 180));
    buildLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (buildLabel);

    presetLabel.setText ("Preset", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    presetLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (presetLabel);

    presetBox.addItem ("All Off", 1);
    presetBox.addItem ("String Quartet", 2);
    presetBox.addItem ("Low Strings", 3);
    presetBox.addItem ("Full Strings", 4);
    presetBox.addItem ("Tutti", 5);
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

    ccMapLabel.setText (
        "Phase 1A.1 CC Map:\n"
        "CC20 = Violin I\n"
        "CC21 = Violin II\n"
        "CC22 = Viola\n"
        "CC23 = Cello\n"
        "CC24 = Double Bass",
        juce::dontSendNotification);
    ccMapLabel.setJustificationType (juce::Justification::centredLeft);
    ccMapLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (210, 220, 230));
    ccMapLabel.setFont (juce::FontOptions (14.0f));
    addAndMakeVisible (ccMapLabel);

    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (245, 195, 90));
    statusLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (statusLabel);

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
}

void OrchConductorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28);

    titleLabel.setBounds (area.removeFromTop (42));
    subtitleLabel.setBounds (area.removeFromTop (24));
    buildLabel.setBounds (area.removeFromTop (22));

    area.removeFromTop (18);

    auto row = area.removeFromTop (42);
    presetLabel.setBounds (row.removeFromLeft (120));
    presetBox.setBounds (row.removeFromLeft (260));

    area.removeFromTop (12);

    sendOnChangeToggle.setBounds (area.removeFromTop (28).withSizeKeepingCentre (240, 24));

    area.removeFromTop (16);

    auto buttonRow = area.removeFromTop (44);
    sendButton.setBounds (buttonRow.removeFromLeft (240).withSizeKeepingCentre (210, 38));
    allOffButton.setBounds (buttonRow.removeFromLeft (240).withSizeKeepingCentre (180, 38));

    area.removeFromTop (28);

    ccMapLabel.setBounds (area.removeFromTop (120));

    area.removeFromTop (18);

    statusLabel.setBounds (area.removeFromTop (32));
}

void OrchConductorAudioProcessorEditor::updateStatus()
{
    const juce::String autoSendText = audioProcessor.getSendOnPresetChange() ? " | Auto-send: On" : " | Auto-send: Off";
    statusLabel.setText ("Selected preset: " + audioProcessor.getPresetName() + autoSendText, juce::dontSendNotification);
}
