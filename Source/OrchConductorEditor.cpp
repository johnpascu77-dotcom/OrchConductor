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

    juce::String makeNumberedPresetLabel (int presetId, const juce::String& label)
    {
        return juce::String (presetId).paddedLeft ('0', 2) + " " + label;
    }

    void addCombiPresetItems (juce::ComboBox& box, const OrchConductorAudioProcessor& processor)
    {
        box.clear (juce::dontSendNotification);

        for (int presetId = 0; presetId <= processor.getMaxCombiPresetId(); ++presetId)
            box.addItem (makeNumberedPresetLabel (presetId, processor.getCombiPresetLabel (presetId)),
                         presetId + 1);
    }

    void addSectionPresetItems (juce::ComboBox& box,
                                const OrchConductorAudioProcessor& processor,
                                OrchConductorAudioProcessor::Section section)
    {
        box.clear (juce::dontSendNotification);

        for (int presetId = 0; presetId <= processor.getMaxSectionPresetId (section); ++presetId)
            box.addItem (makeNumberedPresetLabel (presetId, processor.getSectionPresetLabel (section, presetId)),
                         presetId + 1);
    }
    juce::String getUserFacingCatalogStatus (const OrchConductorAudioProcessor& processor)
    {
        return processor.isRuntimePresetCatalogAuthorityActive()
            ? "Catalog: Runtime JSON active"
            : "Catalog: Factory fallback active";
    }

    juce::String getUserFacingCatalogDetail (const OrchConductorAudioProcessor& processor)
    {
        return processor.isRuntimePresetCatalogAuthorityActive()
            ? "MIDI map and output previews follow catalog values"
            : "MIDI map and output previews use built-in values";
    }
    void appendMidiMapSectionHeader (juce::String& text, const juce::String& sectionName)
    {
        text << sectionName << "\n";
        text << "--------------------------------------------------\n";
    }

    void appendMidiMapRow (juce::String& text,
                           int ccNumber,
                           const juce::String& instrumentName,
                           int value,
                           int activePlayers,
                           int maxPlayers)
    {
        text << "CC" << juce::String (ccNumber)
             << "  " << instrumentName
             << " - value " << juce::String (value)
             << ", players " << juce::String (activePlayers) << "/" << juce::String (maxPlayers)
             << "\n";
    }
    class MidiMapTextComponent final : public juce::Component
    {
    public:
        explicit MidiMapTextComponent (const juce::String& midiMapText)
        {
            textEditor.setMultiLine (true);
            textEditor.setReadOnly (true);
            textEditor.setScrollbarsShown (true);
            textEditor.setCaretVisible (false);
            textEditor.setPopupMenuEnabled (true);
            textEditor.setText (midiMapText, juce::dontSendNotification);
            textEditor.setJustification (juce::Justification::topLeft);
            textEditor.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));

            textEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (20, 28, 36));
            textEditor.setColour (juce::TextEditor::textColourId, juce::Colours::white);
            textEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour::fromRGB (80, 120, 150));
            textEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour::fromRGB (95, 200, 245));
            textEditor.setColour (juce::TextEditor::highlightColourId, juce::Colour::fromRGB (45, 75, 95));

            addAndMakeVisible (textEditor);
            setSize (660, 460);
        }

        void resized() override
        {
            textEditor.setBounds (getLocalBounds());
        }

    private:
        juce::TextEditor textEditor;
    };}

OrchConductorAudioProcessorEditor::OrchConductorAudioProcessorEditor (OrchConductorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (980, 660);

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

    buildLabel.setText ("Build: Phase 9B", juce::dontSendNotification);
    buildLabel.setJustificationType (juce::Justification::centred);
    buildLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (140, 160, 180));
    buildLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (buildLabel);

    combiPresetLabel.setText ("Combi Preset", juce::dontSendNotification);
    styleLabel (combiPresetLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (combiPresetLabel);

    addCombiPresetItems (combiPresetBox, audioProcessor);
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

    addSectionPresetItems (woodwindsPresetBox, audioProcessor, OrchConductorAudioProcessor::Section::woodwinds);
    woodwindsPresetBox.setSelectedId (audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::woodwinds) + 1, juce::dontSendNotification);
    styleComboBox (woodwindsPresetBox, true);
    addAndMakeVisible (woodwindsPresetBox);

    addSectionPresetItems (brassPresetBox, audioProcessor, OrchConductorAudioProcessor::Section::brass);
    brassPresetBox.setSelectedId (audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::brass) + 1, juce::dontSendNotification);
    styleComboBox (brassPresetBox, true);
    addAndMakeVisible (brassPresetBox);

    addSectionPresetItems (percussionPresetBox, audioProcessor, OrchConductorAudioProcessor::Section::percussion);
    percussionPresetBox.setSelectedId (audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::percussion) + 1, juce::dontSendNotification);
    styleComboBox (percussionPresetBox, true);
    addAndMakeVisible (percussionPresetBox);

    addSectionPresetItems (presetBox, audioProcessor, OrchConductorAudioProcessor::Section::strings);
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

    userCombiNameLabel.setText ("User Combi Name", juce::dontSendNotification);
    styleLabel (userCombiNameLabel, juce::Colours::white, 13.0f, juce::Font::plain);
    addAndMakeVisible (userCombiNameLabel);

    userCombiNameEditor.setText ("My Combi", juce::dontSendNotification);
    userCombiNameEditor.setSelectAllWhenFocused (true);
    userCombiNameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
    userCombiNameEditor.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    userCombiNameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour::fromRGB (70, 85, 95));
    userCombiNameEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour::fromRGB (95, 200, 245));
    userCombiNameEditor.setColour (juce::TextEditor::highlightColourId, juce::Colour::fromRGB (45, 75, 95));
    addAndMakeVisible (userCombiNameEditor);

    saveUserCombiButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (45, 75, 95));
    saveUserCombiButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (saveUserCombiButton);

    saveUserCombiButton.onClick = [this]
    {
        auto name = userCombiNameEditor.getText().trim();

        if (name.isEmpty())
            name = "User Combi";

        const auto id = audioProcessor.createUserCombiPresetFromCurrentSections (name);

        if (id >= 0)
        {
            addCombiPresetItems (combiPresetBox, audioProcessor);
            combiPresetBox.setSelectedId (id + 1, juce::sendNotificationSync);

            lastActionText = "Last action: Saved user combi preset: " + name;
        }
        else
        {
            lastActionText = "Last action: Could not save user combi preset";
        }

        updateStatus();
    };

    deleteUserCombiButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (95, 45, 45));
    deleteUserCombiButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (deleteUserCombiButton);

    deleteUserCombiButton.onClick = [this]
    {
        const auto presetId = combiPresetBox.getSelectedId() - 1;
        const auto presetName = audioProcessor.getCombiPresetLabel (presetId);

        if (audioProcessor.deleteUserCombiPreset (presetId))
        {
            addCombiPresetItems (combiPresetBox, audioProcessor);
            combiPresetBox.setSelectedId (audioProcessor.getCombiPresetId() + 1, juce::dontSendNotification);

            lastActionText = "Last action: Deleted user combi preset: " + presetName;
        }
        else
        {
            lastActionText = "Last action: No user combi preset selected for deletion";
        }

        updateStatus();
    };

    exportUserCombisButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (45, 60, 80));
    exportUserCombisButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (exportUserCombisButton);

    exportUserCombisButton.onClick = [this]
    {
        userCombiExportChooser = std::make_unique<juce::FileChooser> (
            "Export User Combi Presets",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("OrchConductorUserCombis.json"),
            "*.json");

        userCombiExportChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                           | juce::FileBrowserComponent::canSelectFiles
                                           | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& chooser)
            {
                const auto file = chooser.getResult();

                if (file == juce::File{})
                    return;

                const auto ok = audioProcessor.writeUserCombiPresetsJsonToFile (file);

                lastActionText = ok
                    ? "Last action: Exported user combi presets to " + file.getFileName()
                    : "Last action: Failed to export user combi presets";

                updateStatus();
            });
    };
    importUserCombisButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (55, 70, 55));
    importUserCombisButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (importUserCombisButton);

    importUserCombisButton.onClick = [this]
    {
        userCombiImportChooser = std::make_unique<juce::FileChooser> (
            "Import User Combi Presets",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
            "*.json");

        userCombiImportChooser->launchAsync (juce::FileBrowserComponent::openMode
                                           | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& chooser)
            {
                const auto file = chooser.getResult();

                if (file == juce::File{})
                    return;

                if (! file.existsAsFile())
                {
                    lastActionText = "Last action: Import failed - file does not exist";
                    updateStatus();
                    return;
                }

                const auto imported = audioProcessor.importUserCombiPresetsFromJson (file.loadFileAsString());

                if (! imported)
                {
                    lastActionText = "Last action: Import failed - invalid user combi JSON";
                    updateStatus();
                    return;
                }

                const auto saved = audioProcessor.saveUserCombiPresetsToUserLibrary();

                addCombiPresetItems (combiPresetBox, audioProcessor);
                combiPresetBox.setSelectedId (audioProcessor.getCombiPresetId() + 1, juce::dontSendNotification);

                lastActionText = saved
                    ? "Last action: Imported user combi presets from " + file.getFileName()
                    : "Last action: Imported user combis, but failed to save library";

                updateStatus();
            });
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
        "Phase 9B: runtime combi workflow UX | CC49 reserved for Harp",
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

    const juce::Rectangle<float> futureArea (48.0f, 476.0f, 884.0f, 80.0f);

    g.setColour (panelColour);
    g.fillRoundedRectangle (futureArea, 6.0f);

    g.setColour (outlineColour);
    g.drawRoundedRectangle (futureArea, 6.0f, 1.0f);

    g.setColour (juce::Colour::fromRGB (120, 140, 155));
    g.setFont (juce::FontOptions (13.0f, juce::Font::plain));
    g.drawText ("Phase 9B: " + getUserFacingCatalogStatus (audioProcessor) + " | " + getUserFacingCatalogDetail (audioProcessor),
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

    area.removeFromTop (8);

    auto userCombiRow = area.removeFromTop (32);
    userCombiNameLabel.setBounds (userCombiRow.removeFromLeft (150));
    userCombiNameEditor.setBounds (userCombiRow.removeFromLeft (260).reduced (0, 2));
    userCombiRow.removeFromLeft (10);
    saveUserCombiButton.setBounds (userCombiRow.removeFromLeft (300).reduced (0, 2));

    area.removeFromTop (6);

    auto userCombiActionsRow = area.removeFromTop (32);
    userCombiActionsRow.removeFromLeft (150);
    deleteUserCombiButton.setBounds (userCombiActionsRow.removeFromLeft (220).reduced (0, 2));
    userCombiActionsRow.removeFromLeft (10);
    exportUserCombisButton.setBounds (userCombiActionsRow.removeFromLeft (220).reduced (0, 2));
    userCombiActionsRow.removeFromLeft (10);
    importUserCombisButton.setBounds (userCombiActionsRow.removeFromLeft (220).reduced (0, 2));

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

    area.removeFromTop (8);

    auto midiMapRow = area.removeFromTop (38);
    midiMapButton.setBounds (midiMapRow.withSizeKeepingCentre (200, 34));

    ccMapLabel.setBounds (48, 578, 884, 24);
    statusLabel.setBounds (48, 606, 884, 28);
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
    ccMapLabel.setText (
        audioProcessor.isCombiModeActive()
            ? "Phase 9B: Combi mode active - section preset controls are overridden until edited | CC49 reserved for Harp"
            : "Phase 9B: Manual section mode - section preset controls define output | CC49 reserved for Harp",
        juce::dontSendNotification);

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

    text << "Full-score MIDI Gate Map\n";
    text << "Runtime status:\n";
    text << "  " << audioProcessor.getRuntimePresetCatalogAuthorityStatus() << "\n\n";

    if (audioProcessor.isCombiModeActive())
    {
        text << "Mode:\n";
        text << "  Combi\n";
        text << "Preset:\n";
        text << "  " << audioProcessor.getCombiPresetName() << "\n\n";
    }
    else
    {
        text << "Mode:\n";
        text << "  Manual Sections\n";
        text << "Presets:\n";
        text << "  Woodwinds:  " << audioProcessor.getSectionPresetLabel (
                    OrchConductorAudioProcessor::Section::woodwinds,
                    audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::woodwinds))
             << "\n";
        text << "  Brass:      " << audioProcessor.getSectionPresetLabel (
                    OrchConductorAudioProcessor::Section::brass,
                    audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::brass))
             << "\n";
        text << "  Percussion: " << audioProcessor.getSectionPresetLabel (
                    OrchConductorAudioProcessor::Section::percussion,
                    audioProcessor.getSectionPresetId (OrchConductorAudioProcessor::Section::percussion))
             << "\n";
        text << "  Strings:    " << audioProcessor.getPresetName()
             << "\n\n";
    }

    text << "Values:\n";
    text << "  Displayed CC values follow the active runtime catalog authority when available.\n\n";

    appendMidiMapSectionHeader (text, "Woodwinds");
    for (int i = 0; i < OrchConductorAudioProcessor::getNumWoodwindsOutputRows(); ++i)
    {
        const auto row = audioProcessor.getWoodwindsOutputRow (i);
        appendMidiMapRow (text, row.ccNumber, row.instrumentName, row.value, row.activePlayers, row.maxPlayers);
    }

    text << "\n";
    appendMidiMapSectionHeader (text, "Brass");
    for (int i = 0; i < OrchConductorAudioProcessor::getNumBrassOutputRows(); ++i)
    {
        const auto row = audioProcessor.getBrassOutputRow (i);
        appendMidiMapRow (text, row.ccNumber, row.instrumentName, row.value, row.activePlayers, row.maxPlayers);
    }

    text << "\n";
    appendMidiMapSectionHeader (text, "Melodic Percussion");
    for (int i = 0; i < OrchConductorAudioProcessor::getNumPercussionOutputRows(); ++i)
    {
        const auto row = audioProcessor.getPercussionOutputRow (i);
        appendMidiMapRow (text, row.ccNumber, row.instrumentName, row.value, row.activePlayers, row.maxPlayers);
    }

    appendMidiMapSectionHeader (text, "Reserved");
    text << "CC49  Harp - reserved\n";

    text << "\n";
    appendMidiMapSectionHeader (text, "Strings");
    for (int i = 0; i < OrchConductorAudioProcessor::getNumOutputRows(); ++i)
    {
        const auto row = audioProcessor.getOutputRow (i);
        appendMidiMapRow (text, row.ccNumber, row.instrumentName, row.value, row.activePlayers, row.maxPlayers);
    }

    text << "\nUse these CC numbers as the assigned CC Gate values in each OrchGate instance.";

    return text;
}

void OrchConductorAudioProcessorEditor::showMidiMap()
{
    auto* window = new juce::AlertWindow (
        "OrchConductor MIDI Map",
        "Scrollable runtime-backed MIDI map",
        juce::AlertWindow::InfoIcon);

    window->addCustomComponent (new MidiMapTextComponent (buildMidiMapText()));
    window->addButton ("OK", 0, juce::KeyPress (juce::KeyPress::returnKey));
    window->setColour (juce::AlertWindow::backgroundColourId, juce::Colour::fromRGB (28, 40, 48));
    window->setColour (juce::AlertWindow::textColourId, juce::Colours::white);
    window->enterModalState (true, nullptr, true);
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



















