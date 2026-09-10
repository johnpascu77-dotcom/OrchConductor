#include "OrchConductorEditor.h"
#include "OrchConductorBuildInfo.h"

#include <cmath>

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

    juce::String formatMetadataValue (double value)
    {
        return juce::String (value, 2);
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
    setResizable (true, true);
    setResizeLimits (900, 700, 1400, 1200);
    setSize (980, 894);

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

    // orchConductorBuildTimestamp is regenerated on every single build (see
    // cmake/GenerateOrchConductorBuildInfo.cmake) - a hand-maintained phase
    // tag here can't answer "is this actually the build I just installed",
    // a fresh timestamp always can.
    buildLabel.setText (juce::String ("Build: ") + orchConductorBuildTimestamp, juce::dontSendNotification);
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

        // Picking a combi (or 00) is an authority choice; follow it unless the
        // user is deliberately in Narrative Scan.
        if (audioProcessor.getAuthorityMode() != OrchConductorAudioProcessor::AuthorityMode::narrativeScan)
        {
            audioProcessor.setAuthorityMode (
                selected == 0 ? OrchConductorAudioProcessor::AuthorityMode::manualSections
                              : OrchConductorAudioProcessor::AuthorityMode::combiPreset);
        }

        updateNarrativeMetadataDisplay();
        updateNarrativeScanControls();
        updateStatus();
    };

    authorityModeLabel.setText ("Authority Mode", juce::dontSendNotification);
    styleLabel (authorityModeLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (authorityModeLabel);

    authorityModeBox.addItem ("Manual Sections", 1);
    authorityModeBox.addItem ("Combi Preset", 2);
    authorityModeBox.addItem ("Narrative Scan", 3);
    authorityModeBox.setSelectedId (static_cast<int> (audioProcessor.getAuthorityMode()) + 1, juce::dontSendNotification);
    styleComboBox (authorityModeBox, true);
    addAndMakeVisible (authorityModeBox);

    authorityModeBox.onChange = [this]
    {
        const auto mode = static_cast<OrchConductorAudioProcessor::AuthorityMode> (authorityModeBox.getSelectedId() - 1);
        audioProcessor.setAuthorityMode (mode);

        // Keep the combi/section controls coherent with the chosen authority so
        // the status line never contradicts the selector. Narrative Scan leaves
        // the combi box alone (it does not drive output in that mode).
        if (mode == OrchConductorAudioProcessor::AuthorityMode::manualSections)
        {
            combiPresetBox.setSelectedId (1, juce::sendNotificationSync); // -> Manual Sections (combi 0)
        }
        else if (mode == OrchConductorAudioProcessor::AuthorityMode::combiPreset
                 && ! audioProcessor.isCombiModeActive())
        {
            combiPresetBox.setSelectedId (
                static_cast<int> (OrchConductorAudioProcessor::CombiPreset::utilityFullOrchestra) + 1,
                juce::sendNotificationSync);
        }

        updateNarrativeScanControls();
        updateNarrativeMetadataDisplay();
        updateStatus();
    };

    narrativeScanLabel.setText ("Narrative Scan", juce::dontSendNotification);
    styleLabel (narrativeScanLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    addAndMakeVisible (narrativeScanLabel);

    rebuildNarrativeLaneItems();
    narrativeLaneBox.setSelectedId (audioProcessor.getNarrativeLaneIndex() + 1, juce::dontSendNotification);
    styleComboBox (narrativeLaneBox, true);
    addAndMakeVisible (narrativeLaneBox);

    narrativeLaneBox.onChange = [this]
    {
        audioProcessor.setNarrativeLaneIndex (narrativeLaneBox.getSelectedId() - 1);
        updateNarrativeScanControls();
        updateStatus();
    };

    narrativePositionSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    narrativePositionSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 22);
    narrativePositionSlider.setRange (0.0, 1.0, 0.001);
    narrativePositionSlider.setValue (audioProcessor.getNarrativePosition(), juce::dontSendNotification);
    narrativePositionSlider.setColour (juce::Slider::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
    narrativePositionSlider.setColour (juce::Slider::trackColourId, juce::Colour::fromRGB (95, 200, 245));
    narrativePositionSlider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    narrativePositionSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    narrativePositionSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB (28, 36, 46));
    narrativePositionSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB (70, 85, 95));
    addAndMakeVisible (narrativePositionSlider);

    narrativePositionSlider.onValueChange = [this]
    {
        audioProcessor.setNarrativePosition (narrativePositionSlider.getValue());
        updateNarrativeScanControls();
        updateStatus();
    };

    narrativeScanStatusLabel.setJustificationType (juce::Justification::centred);
    narrativeScanStatusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (205, 220, 230));
    narrativeScanStatusLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (narrativeScanStatusLabel);

    exportNarrativeLibraryButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (45, 60, 80));
    exportNarrativeLibraryButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (exportNarrativeLibraryButton);

    exportNarrativeLibraryButton.onClick = [this]
    {
        narrativeLibraryExportChooser = std::make_unique<juce::FileChooser> (
            "Export Lane Library JSON (editable template)",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("OrchConductorLibrary.json"),
            "*.json");

        narrativeLibraryExportChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                                 | juce::FileBrowserComponent::canSelectFiles
                                                 | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& chooser)
            {
                const auto file = chooser.getResult();

                if (file == juce::File{})
                    return;

                const auto ok = audioProcessor.exportNarrativeLibraryTemplateToFile (file);

                lastActionText = ok
                    ? "Last action: Exported lane library template to " + file.getFileName()
                    : "Last action: Failed to export lane library template";

                updateStatus();
            });
    };

    importNarrativeLibraryButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (55, 70, 55));
    importNarrativeLibraryButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (importNarrativeLibraryButton);

    importNarrativeLibraryButton.onClick = [this]
    {
        narrativeLibraryImportChooser = std::make_unique<juce::FileChooser> (
            "Import Lane Library JSON",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
            "*.json");

        narrativeLibraryImportChooser->launchAsync (juce::FileBrowserComponent::openMode
                                                 | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& chooser)
            {
                const auto file = chooser.getResult();

                if (file == juce::File{})
                    return;

                if (! file.existsAsFile())
                {
                    lastActionText = "Last action: Lane library import failed - file does not exist";
                    updateStatus();
                    return;
                }

                const auto imported = audioProcessor.importNarrativeLibraryFromFile (file);

                rebuildNarrativeLaneItems();
                narrativeLaneBox.setSelectedId (audioProcessor.getNarrativeLaneIndex() + 1, juce::dontSendNotification);
                updateNarrativeScanControls();
                updateNarrativeMetadataDisplay();

                lastActionText = imported
                    ? "Last action: Imported lane library from " + file.getFileName()
                    : "Last action: Lane library import rejected - see the line above";

                updateStatus();
            });
    };

    narrativeLibrarySourceLabel.setJustificationType (juce::Justification::centred);
    narrativeLibrarySourceLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (150, 170, 185));
    narrativeLibrarySourceLabel.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (narrativeLibrarySourceLabel);

    sectionPresetsLabel.setText ("Manual Section Presets", juce::dontSendNotification);
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

    inputPassthroughLabel.setText ("Input Passthrough", juce::dontSendNotification);
    styleLabel (inputPassthroughLabel, juce::Colours::white, 13.0f, juce::Font::bold);
    addAndMakeVisible (inputPassthroughLabel);

    inputPassthroughBox.addItem ("Off", 1);
    inputPassthroughBox.addItem ("Control CCs (>= 105)", 2);
    inputPassthroughBox.addItem ("All", 3);
    inputPassthroughBox.setSelectedId (
        static_cast<int> (audioProcessor.getInputPassthroughMode()) + 1, juce::dontSendNotification);
    styleComboBox (inputPassthroughBox, true);
    addAndMakeVisible (inputPassthroughBox);

    inputPassthroughBox.onChange = [this]
    {
        audioProcessor.setInputPassthroughModeFromUI (
            static_cast<OrchConductorAudioProcessor::InputPassthroughMode> (inputPassthroughBox.getSelectedId() - 1));
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

    auto setupCombiCcSlider = [this] (juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        styleLabel (label, juce::Colour::fromRGB (205, 220, 230), 12.0f, juce::Font::plain);
        addAndMakeVisible (label);

        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 22);
        slider.setRange (-1.0, 127.0, 1.0);
        slider.setValue (-1.0, juce::dontSendNotification);
        slider.textFromValueFunction = [] (double v)
        {
            return v < 0.0 ? juce::String ("Off") : juce::String (juce::roundToInt (v));
        };
        slider.valueFromTextFunction = [] (const juce::String& t)
        {
            return t.trim().equalsIgnoreCase ("off") ? -1.0 : (double) t.getIntValue();
        };
        slider.setColour (juce::Slider::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
        slider.setColour (juce::Slider::trackColourId, juce::Colour::fromRGB (95, 200, 245));
        slider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
        slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB (28, 36, 46));
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB (70, 85, 95));
        addAndMakeVisible (slider);
    };

    setupCombiCcSlider (userCombiHarpSlider, userCombiHarpLabel, "Harp (CC49)");
    setupCombiCcSlider (userCombiPianoSlider, userCombiPianoLabel, "Piano (CC55)");

    saveUserCombiButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (45, 75, 95));
    saveUserCombiButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible (saveUserCombiButton);

    saveUserCombiButton.onClick = [this]
    {
        auto name = userCombiNameEditor.getText().trim();

        if (name.isEmpty()
            || name.equalsIgnoreCase ("My Combi")
            || name.equalsIgnoreCase ("User Combi"))
        {
            name = audioProcessor.createUserCombiNameFromCurrentSections();
            userCombiNameEditor.setText (name, juce::dontSendNotification);
        }

        const int harpValue = juce::roundToInt (userCombiHarpSlider.getValue());
        const int pianoValue = juce::roundToInt (userCombiPianoSlider.getValue());

        const auto id = audioProcessor.createUserCombiPresetFromCurrentSections (name, harpValue, pianoValue);

        if (id >= 0)
        {
            addCombiPresetItems (combiPresetBox, audioProcessor);
            combiPresetBox.setSelectedId (id + 1, juce::sendNotificationSync);

            userCombiHarpSlider.setValue (-1.0, juce::dontSendNotification);
            userCombiPianoSlider.setValue (-1.0, juce::dontSendNotification);

            juce::String extras;
            if (harpValue >= 0) extras << " Harp " << harpValue;
            if (pianoValue >= 0) extras << " Piano " << pianoValue;

            lastActionText = "Last action: Saved user combi preset: " + name + extras;
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
                updateNarrativeMetadataDisplay();

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

    narrativeMetadataLabel.setText ("Narrative Metadata", juce::dontSendNotification);
    narrativeMetadataLabel.setJustificationType (juce::Justification::centred);
    styleLabel (narrativeMetadataLabel, juce::Colour::fromRGB (245, 245, 245), 14.0f, juce::Font::bold);
    addAndMakeVisible (narrativeMetadataLabel);

    narrativeMetadataValueLabel.setJustificationType (juce::Justification::centred);
    narrativeMetadataValueLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (205, 220, 230));
    narrativeMetadataValueLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (narrativeMetadataValueLabel);

    ccMapLabel.setText (
        "Phase 10G: user-combi Harp/Piano overrides | CC49 Harp, CC55 Piano (user-combi only)",
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
    updateNarrativeMetadataDisplay();
    updateNarrativeScanControls();
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

    auto authorityRow = area.removeFromTop (34);
    authorityModeLabel.setBounds (authorityRow.removeFromLeft (150));
    authorityModeBox.setBounds (authorityRow.removeFromLeft (300));

    area.removeFromTop (8);

    auto userCombiRow = area.removeFromTop (32);
    userCombiNameLabel.setBounds (userCombiRow.removeFromLeft (150));
    userCombiNameEditor.setBounds (userCombiRow.removeFromLeft (260).reduced (0, 2));
    userCombiRow.removeFromLeft (10);
    saveUserCombiButton.setBounds (userCombiRow.removeFromLeft (300).reduced (0, 2));

    area.removeFromTop (4);

    auto userCombiCcRow = area.removeFromTop (28);
    userCombiCcRow.removeFromLeft (150);
    userCombiHarpLabel.setBounds (userCombiCcRow.removeFromLeft (80));
    userCombiHarpSlider.setBounds (userCombiCcRow.removeFromLeft (220).reduced (0, 2));
    userCombiCcRow.removeFromLeft (24);
    userCombiPianoLabel.setBounds (userCombiCcRow.removeFromLeft (84));
    userCombiPianoSlider.setBounds (userCombiCcRow.removeFromLeft (220).reduced (0, 2));

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

    area.removeFromTop (14);

    narrativeScanLabel.setBounds (area.removeFromTop (22));

    area.removeFromTop (4);

    auto narrativeScanRow = area.removeFromTop (34);
    narrativeLaneBox.setBounds (narrativeScanRow.removeFromLeft (300));
    narrativeScanRow.removeFromLeft (16);
    narrativePositionSlider.setBounds (narrativeScanRow.removeFromLeft (560));

    narrativeScanStatusLabel.setBounds (area.removeFromTop (20));

    area.removeFromTop (6);

    auto narrativeLibraryRow = area.removeFromTop (30);
    exportNarrativeLibraryButton.setBounds (narrativeLibraryRow.removeFromLeft (220).reduced (0, 2));
    narrativeLibraryRow.removeFromLeft (12);
    importNarrativeLibraryButton.setBounds (narrativeLibraryRow.removeFromLeft (220).reduced (0, 2));

    narrativeLibrarySourceLabel.setBounds (area.removeFromTop (18));

    area.removeFromTop (12);

    narrativeMetadataLabel.setBounds (area.removeFromTop (22));
    narrativeMetadataValueLabel.setBounds (area.removeFromTop (64).reduced (8, 0));

    area.removeFromTop (10);

    auto buttonRow = area.removeFromTop (42);
    sendButton.setBounds (buttonRow.removeFromLeft (280).withSizeKeepingCentre (240, 36));
    allOffButton.setBounds (buttonRow.removeFromLeft (240).withSizeKeepingCentre (190, 36));
    sendOnChangeToggle.setBounds (buttonRow.removeFromLeft (300).withSizeKeepingCentre (260, 24));

    area.removeFromTop (8);

    auto midiMapRow = area.removeFromTop (38);
    midiMapButton.setBounds (midiMapRow.withSizeKeepingCentre (200, 34));

    area.removeFromTop (6);
    {
        auto row = area.removeFromTop (26);
        inputPassthroughLabel.setBounds (row.removeFromLeft (140));
        inputPassthroughBox.setBounds (row.removeFromLeft (260));
    }

    // Footer anchored to the window bottom so shrinking the editor squeezes the
    // middle, not the status line.
    auto footer = getLocalBounds().reduced (48, 0);
    footer.removeFromBottom (16);
    statusLabel.setBounds (footer.removeFromBottom (26));
    footer.removeFromBottom (4);
    ccMapLabel.setBounds (footer.removeFromBottom (22));
}


void OrchConductorAudioProcessorEditor::timerCallback()
{
    // Phase 2A: Bitwig may restore plugin state after the editor has already been created.
    // Keep visible controls synced to processor state without firing onChange callbacks.

    const int combiId = audioProcessor.getCombiPresetId() + 1;
    if (combiPresetBox.getSelectedId() != combiId)
    {
        combiPresetBox.setSelectedId (combiId, juce::dontSendNotification);
        updateNarrativeMetadataDisplay();
    }

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

    const int passthroughId = static_cast<int> (audioProcessor.getInputPassthroughMode()) + 1;
    if (inputPassthroughBox.getSelectedId() != passthroughId)
        inputPassthroughBox.setSelectedId (passthroughId, juce::dontSendNotification);

    const int authorityId = static_cast<int> (audioProcessor.getAuthorityMode()) + 1;
    if (authorityModeBox.getSelectedId() != authorityId)
        authorityModeBox.setSelectedId (authorityId, juce::dontSendNotification);

    const int laneId = audioProcessor.getNarrativeLaneIndex() + 1;
    if (narrativeLaneBox.getSelectedId() != laneId && narrativeLaneBox.getNumItems() >= laneId)
        narrativeLaneBox.setSelectedId (laneId, juce::dontSendNotification);

    if (std::abs (narrativePositionSlider.getValue() - audioProcessor.getNarrativePosition()) > 0.0005)
        narrativePositionSlider.setValue (audioProcessor.getNarrativePosition(), juce::dontSendNotification);

    updateNarrativeScanControls();
    updateStatus();
}

void OrchConductorAudioProcessorEditor::updateNarrativeMetadataDisplay()
{
    orchconductor::NarrativeMetadata metadata;

    const auto presetId = audioProcessor.getCombiPresetId();

    if (! audioProcessor.getCombiPresetNarrativeMetadata (presetId, metadata))
    {
        juce::String unavailableText;

        if (presetId == 0)
        {
            unavailableText = "Narrative metadata unavailable in Manual Sections mode.";
        }
        else if (! audioProcessor.isUserCombiPresetId (presetId)
                 && ! audioProcessor.isRuntimePresetCatalogAuthorityActive())
        {
            unavailableText =
                "Narrative metadata unavailable for factory fallback combis.\n"
                "Runtime JSON catalog metadata is required.";
        }
        else
        {
            unavailableText = "Narrative metadata unavailable for selected combi.";
        }

        narrativeMetadataValueLabel.setText (unavailableText, juce::dontSendNotification);
        return;
    }

    const auto registerName = metadata.registerName.isNotEmpty() ? metadata.registerName : "unspecified";
    const auto role = metadata.role.isNotEmpty() ? metadata.role : "unspecified";
    const auto transitionBehavior = metadata.transitionBehavior.isNotEmpty() ? metadata.transitionBehavior : "unspecified";
    const auto narrativeLane = metadata.narrativeLane.isNotEmpty() ? metadata.narrativeLane : "unspecified";

    juce::String text;
    text << "Energy " << formatMetadataValue (metadata.energy)
         << " | Density " << formatMetadataValue (metadata.density)
         << " | Brightness " << formatMetadataValue (metadata.brightness)
         << " | Weight " << formatMetadataValue (metadata.weight)
         << " | Tension " << formatMetadataValue (metadata.tension)
         << "\n"
         << "Register: " << registerName
         << " | Role: " << role
         << "\n"
         << "Transition: " << transitionBehavior
         << " | Lane: " << narrativeLane;

    narrativeMetadataValueLabel.setText (text, juce::dontSendNotification);
}

void OrchConductorAudioProcessorEditor::updateStatus()
{
    const bool narrative =
        audioProcessor.getAuthorityMode() == OrchConductorAudioProcessor::AuthorityMode::narrativeScan;

    const juce::String authorityText = narrative
        ? "Authority: Narrative Scan"
        : (audioProcessor.isCombiModeActive() ? "Authority: Combi Preset"
                                              : "Authority: Manual Sections");

    ccMapLabel.setText (
        "Phase 10G | " + authorityText + " | " + getUserFacingCatalogStatus (audioProcessor)
        + " | CC49 Harp / CC55 Piano (user-combi only)",
        juce::dontSendNotification);

    const juce::String autoSendText = audioProcessor.getSendOnPresetChange() ? " | Auto-send: On" : " | Auto-send: Off";
    const juce::String activePlayersText = " | Active players: " + juce::String (audioProcessor.getTotalActivePlayers());
    const juce::String sendFeedbackText = " | " + lastActionText + " | Send requests: " + juce::String (sendRequestCount);

    if (narrative)
    {
        const int laneIndex = audioProcessor.getNarrativeLaneIndex();
        const int combiId = audioProcessor.getResolvedNarrativeCombiId();

        const juce::String resolvedText = combiId >= 0
            ? audioProcessor.getCombiPresetLabel (combiId)
            : juce::String ("(no lane resolved)");

        statusLabel.setText (
            "Narrative Scan: " + audioProcessor.getNarrativeLaneLabel (laneIndex)
            + " @ " + juce::String (juce::roundToInt (audioProcessor.getNarrativePosition() * 100.0)) + "%"
            + " -> " + resolvedText
            + activePlayersText
            + " | Harp/Piano: factory default (0)"
            + autoSendText
            + sendFeedbackText,
            juce::dontSendNotification);

        return;
    }

    if (audioProcessor.isCombiModeActive())
    {
        const juce::String harpPianoText =
            " | Harp: " + juce::String (audioProcessor.getHarpCcValue())
            + " | Piano: " + juce::String (audioProcessor.getPianoCcValue());

        statusLabel.setText (
            "Combi active: " + audioProcessor.getCombiPresetName()
            + activePlayersText
            + harpPianoText
            + autoSendText
            + sendFeedbackText,
            juce::dontSendNotification);

        return;
    }

    statusLabel.setText (
        "Manual Sections | Strings: " + audioProcessor.getPresetName()
        + activePlayersText
        + " | Harp/Piano: off (manual mode)"
        + autoSendText
        + sendFeedbackText,
        juce::dontSendNotification);
}

void OrchConductorAudioProcessorEditor::rebuildNarrativeLaneItems()
{
    narrativeLaneBox.clear (juce::dontSendNotification);

    const int count = audioProcessor.getNarrativeLaneCount();

    if (count <= 0)
    {
        narrativeLaneBox.addItem ("(no narrative lanes)", 1);
        return;
    }

    for (int i = 0; i < count; ++i)
    {
        const auto label = audioProcessor.getNarrativeLaneLabel (i);

        narrativeLaneBox.addItem (
            juce::String (i).paddedLeft ('0', 2) + " "
            + (label.isNotEmpty() ? label : juce::String ("Lane ") + juce::String (i)),
            i + 1);
    }
}

void OrchConductorAudioProcessorEditor::updateNarrativeScanControls()
{
    narrativeLibrarySourceLabel.setText (audioProcessor.getNarrativeLibrarySourceStatus(),
                                         juce::dontSendNotification);

    const bool narrative =
        audioProcessor.getAuthorityMode() == OrchConductorAudioProcessor::AuthorityMode::narrativeScan;

    styleComboBox (narrativeLaneBox, narrative);
    narrativePositionSlider.setEnabled (narrative);

    if (! narrative)
    {
        narrativeScanStatusLabel.setText (
            "Narrative Scan inactive - set Authority Mode to Narrative Scan to drive output from a lane.",
            juce::dontSendNotification);
        return;
    }

    const int laneIndex = audioProcessor.getNarrativeLaneIndex();
    const int pointIndex = audioProcessor.getResolvedNarrativeLanePointIndex();
    const int combiId = audioProcessor.getResolvedNarrativeCombiId();

    if (combiId < 0)
    {
        narrativeScanStatusLabel.setText (
            "Lane " + juce::String (laneIndex) + " has no resolvable points.",
            juce::dontSendNotification);
        return;
    }

    const juce::String pointLabel = audioProcessor.getNarrativeLanePointLabel (laneIndex, pointIndex);
    const int fieldIndex = audioProcessor.getLastSentFieldSelectIndex();

    narrativeScanStatusLabel.setText (
        "Resolved -> " + audioProcessor.getCombiPresetLabel (combiId)
        + (pointLabel.isNotEmpty() ? juce::String ("  (") + pointLabel + ")" : juce::String())
        + (fieldIndex >= 0 ? juce::String ("  |  field #") + juce::String (fieldIndex) : juce::String()),
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

    appendMidiMapSectionHeader (text, "Harp / Piano (user-combi override only)");
    text << "CC49  Harp   - value " << audioProcessor.getHarpCcValue()
         << " (no manual control; set via a user combi's harpValue override)\n";
    text << "CC55  Piano  - value " << audioProcessor.getPianoCcValue()
         << " (no manual control; set via a user combi's pianoValue override)\n";

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


