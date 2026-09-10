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
    };

    // A viewport content holder that forwards wheel / two-finger scroll to its
    // parent Viewport even when the pointer is over a child control (a Label
    // passes it through anyway; a Slider would otherwise eat it). Sliders still
    // adjust on wheel only when the pointer is directly over them AND a modifier
    // isn't held - handled by leaving the slider's own wheel behaviour intact;
    // this just guarantees the "scroll over anything" case works.
    class WheelForwardingComponent final : public juce::Component
    {
    public:
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
        {
            if (auto* vp = findParentComponentOfClass<juce::Viewport>())
            {
                auto pos = vp->getViewPosition();
                pos.y -= juce::roundToInt ((wheel.deltaY != 0.0f ? wheel.deltaY : wheel.deltaX) * 110.0f);
                vp->setViewPosition (pos);
            }
        }
    };

    // The "Combi Grid" tab: every instrument's exact CC value in one scrollable
    // grid, saved as a user combi's explicitCcValues (which override every
    // section preset). Covers the whole editor area when shown.
    class InstrumentGridComponent final : public juce::Component
    {
    public:
        std::function<void()> onBack;
        std::function<void()> onSaved;

        explicit InstrumentGridComponent (OrchConductorAudioProcessor& proc)
            : processor (proc)
        {
            setOpaque (true);

            auto styleButton = [] (juce::TextButton& b, juce::Colour c)
            {
                b.setColour (juce::TextButton::buttonColourId, c);
                b.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
            };

            titleLabel.setText ("Instrument Combi Grid", juce::dontSendNotification);
            titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
            addAndMakeVisible (titleLabel);

            hintLabel.setText ("Every instrument's exact CC value. Save as a User Combi - it overrides all section presets.",
                               juce::dontSendNotification);
            hintLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (160, 175, 190));
            hintLabel.setFont (juce::FontOptions (12.0f));
            addAndMakeVisible (hintLabel);

            styleButton (backButton, juce::Colour::fromRGB (55, 60, 70));
            backButton.setButtonText ("< Back to Conductor");
            backButton.onClick = [this] { if (onBack) onBack(); };
            addAndMakeVisible (backButton);

            nameEditor.setText ("Grid Combi", juce::dontSendNotification);
            nameEditor.setSelectAllWhenFocused (true);
            nameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
            nameEditor.setColour (juce::TextEditor::textColourId, juce::Colours::white);
            nameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour::fromRGB (70, 85, 95));
            addAndMakeVisible (nameEditor);

            styleButton (saveNewButton, juce::Colour::fromRGB (45, 75, 95));
            saveNewButton.setButtonText ("Save as New Combi");
            saveNewButton.onClick = [this] { saveAs (-1); };
            addAndMakeVisible (saveNewButton);

            styleButton (updateButton, juce::Colour::fromRGB (55, 70, 55));
            updateButton.setButtonText ("Update Selected Combi");
            updateButton.setEnabled (false);
            updateButton.onClick = [this] { saveAs (editingId); };
            addAndMakeVisible (updateButton);

            loadLabel.setText ("Load / seed:", juce::dontSendNotification);
            loadLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            loadLabel.setFont (juce::FontOptions (13.0f));
            addAndMakeVisible (loadLabel);

            loadBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
            loadBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
            loadBox.setColour (juce::ComboBox::outlineColourId, juce::Colour::fromRGB (70, 85, 95));
            loadBox.onChange = [this] { loadSelected(); };
            addAndMakeVisible (loadBox);

            styleButton (seedButton, juce::Colour::fromRGB (45, 60, 80));
            seedButton.setButtonText ("Seed from Current Output");
            seedButton.onClick = [this] { seedFromCurrent(); };
            addAndMakeVisible (seedButton);

            styleButton (randomButton, juce::Colour::fromRGB (70, 55, 85));
            randomButton.setButtonText ("Randomize");
            randomButton.onClick = [this] { showRandomMenu(); };
            addAndMakeVisible (randomButton);

            styleButton (clearButton, juce::Colour::fromRGB (70, 55, 55));
            clearButton.setButtonText ("Clear");
            clearButton.onClick = [this]
            {
                for (auto* s : sliders) s->setValue (0.0, juce::dontSendNotification);
                markManualEdit();
            };
            addAndMakeVisible (clearButton);

            statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (245, 195, 90));
            statusLabel.setFont (juce::FontOptions (12.0f));
            addAndMakeVisible (statusLabel);

            const int count = OrchConductorAudioProcessor::getInstrumentSlotCount();
            juce::String lastSection;

            for (int i = 0; i < count; ++i)
            {
                const auto section = processor.getInstrumentSlotSectionName (i);

                if (section != lastSection)
                {
                    auto* h = new juce::Label();
                    h->setText (section, juce::dontSendNotification);
                    h->setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
                    h->setFont (juce::FontOptions (13.0f, juce::Font::bold));
                    content.addAndMakeVisible (h);
                    sectionHeaders.add (h);
                    sectionHeaderBeforeSlot.add (i);
                    lastSection = section;
                }

                auto* nameL = new juce::Label();
                nameL->setText (processor.getInstrumentSlotName (i)
                                + "  (CC" + juce::String (processor.getInstrumentSlotCc (i)) + ")",
                                juce::dontSendNotification);
                nameL->setColour (juce::Label::textColourId, juce::Colours::white);
                nameL->setFont (juce::FontOptions (13.0f));
                content.addAndMakeVisible (nameL);
                rowLabels.add (nameL);

                auto* s = new juce::Slider();
                s->setSliderStyle (juce::Slider::LinearHorizontal);
                s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
                s->setRange (0.0, 127.0, 1.0);
                s->setValue (processor.getInstrumentSlotCurrentValue (i), juce::dontSendNotification);
                s->setColour (juce::Slider::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
                s->setColour (juce::Slider::trackColourId, juce::Colour::fromRGB (95, 200, 245));
                s->setColour (juce::Slider::thumbColourId, juce::Colours::white);
                s->setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
                s->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB (28, 36, 46));
                s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB (70, 85, 95));
                s->onValueChange = [this] { markManualEdit(); };
                content.addAndMakeVisible (s);
                sliders.add (s);
            }

            viewport.setViewedComponent (&content, false);
            viewport.setScrollBarsShown (true, false);
            addAndMakeVisible (viewport);

            refreshLoadBox();
        }

        void refreshLoadBox()
        {
            const int keep = loadBox.getSelectedId();
            loadBox.clear (juce::dontSendNotification);
            loadBox.addItem ("(pick a combi)", 1);

            const int maxId = processor.getMaxCombiPresetId();

            for (int id = 0; id <= maxId; ++id)
            {
                const auto label = processor.getCombiPresetLabel (id);

                if (label == "Unknown Combi")
                    continue;

                loadBox.addItem ((processor.isUserCombiPresetId (id) ? juce::String ("[User] ") : juce::String())
                                     + label,
                                 id + 2);
            }

            loadBox.setSelectedId (keep > 0 ? keep : 1, juce::dontSendNotification);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (juce::Colour::fromRGB (22, 30, 38));
        }

        void visibilityChanged() override
        {
            if (isVisible())
                refreshLoadBox();
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (18, 12);

            auto headerRow = r.removeFromTop (30);
            backButton.setBounds (headerRow.removeFromLeft (170));
            headerRow.removeFromLeft (16);
            titleLabel.setBounds (headerRow);

            hintLabel.setBounds (r.removeFromTop (18));
            r.removeFromTop (8);

            auto actionRow = r.removeFromTop (30);
            nameEditor.setBounds (actionRow.removeFromLeft (200).reduced (0, 2));
            actionRow.removeFromLeft (8);
            saveNewButton.setBounds (actionRow.removeFromLeft (170).reduced (0, 2));
            actionRow.removeFromLeft (8);
            updateButton.setBounds (actionRow.removeFromLeft (190).reduced (0, 2));

            r.removeFromTop (6);

            auto loadRow = r.removeFromTop (30);
            loadLabel.setBounds (loadRow.removeFromLeft (90));
            loadBox.setBounds (loadRow.removeFromLeft (300).reduced (0, 2));
            loadRow.removeFromLeft (10);
            seedButton.setBounds (loadRow.removeFromLeft (190).reduced (0, 2));
            loadRow.removeFromLeft (8);
            randomButton.setBounds (loadRow.removeFromLeft (120).reduced (0, 2));
            loadRow.removeFromLeft (8);
            clearButton.setBounds (loadRow.removeFromLeft (80).reduced (0, 2));

            r.removeFromTop (6);
            statusLabel.setBounds (r.removeFromTop (18));
            r.removeFromTop (6);

            viewport.setBounds (r);
            layoutContent();
        }

        void layoutContent()
        {
            const int rowH = 26;
            const int headerH = 24;
            const int width = juce::jmax (320, viewport.getWidth() - 16);

            int y = 0;
            int headerIdx = 0;

            for (int i = 0; i < sliders.size(); ++i)
            {
                if (headerIdx < sectionHeaderBeforeSlot.size() && sectionHeaderBeforeSlot[headerIdx] == i)
                {
                    sectionHeaders[headerIdx]->setBounds (4, y + 4, width - 8, headerH - 4);
                    y += headerH;
                    ++headerIdx;
                }

                rowLabels[i]->setBounds (8, y, 210, rowH);
                sliders[i]->setBounds (224, y + 2, width - 232, rowH - 4);
                y += rowH;
            }

            content.setSize (width, y + 8);
        }

    private:
        void markManualEdit()
        {
            if (editingId < 0)
                statusLabel.setText ("Unsaved grid - Save as New Combi.", juce::dontSendNotification);
            else
                statusLabel.setText ("Editing " + processor.getCombiPresetLabel (editingId)
                                         + " - Update, or Save as New.",
                                     juce::dontSendNotification);
        }

        void setAllSliders (const std::function<int (int slot)>& source)
        {
            for (int i = 0; i < sliders.size(); ++i)
                sliders[i]->setValue (juce::jlimit (0, 127, source (i)), juce::dontSendNotification);
        }

        void seedFromCurrent()
        {
            setAllSliders ([this] (int i) { return processor.getInstrumentSlotCurrentValue (i); });
            editingId = -1;
            updateButton.setEnabled (false);
            loadBox.setSelectedId (1, juce::dontSendNotification);
            statusLabel.setText ("Seeded from the current OrchConductor output. Save as New Combi.",
                                 juce::dontSendNotification);
        }

        void showRandomMenu()
        {
            juce::PopupMenu m;
            m.addSectionHeader ("Random starting point (orchestration-aware)");
            m.addItem (1, "Balanced");
            m.addItem (2, "Feature a section");
            m.addItem (3, "Sparse / chamber");
            m.addItem (4, "Tutti");

            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (randomButton),
                [this] (int choice)
                {
                    if (choice == 0)
                        return;

                    const auto style = choice == 2 ? OrchConductorAudioProcessor::GridRandomStyle::feature
                                     : choice == 3 ? OrchConductorAudioProcessor::GridRandomStyle::sparse
                                     : choice == 4 ? OrchConductorAudioProcessor::GridRandomStyle::tutti
                                                   : OrchConductorAudioProcessor::GridRandomStyle::balanced;

                    const auto values = processor.generateRandomGridCombi (
                        style, juce::Random::getSystemRandom().nextInt64());

                    setAllSliders ([&] (int i)
                    {
                        const int cc = processor.getInstrumentSlotCc (i);

                        for (const auto& pv : values)
                            if (pv.ccNumber == cc)
                                return pv.value;

                        return 0;
                    });

                    editingId = -1;
                    updateButton.setEnabled (false);
                    loadBox.setSelectedId (1, juce::dontSendNotification);
                    statusLabel.setText ("Random grid - tweak and Save as New Combi. Randomize again for another.",
                                         juce::dontSendNotification);
                });
        }

        void loadSelected()
        {
            const int itemId = loadBox.getSelectedId();

            if (itemId <= 1)
                return;

            const int presetId = itemId - 2;
            const auto explicitValues = processor.getUserCombiExplicitCcValues (presetId);

            setAllSliders ([&] (int i)
            {
                const int cc = processor.getInstrumentSlotCc (i);

                for (const auto& ev : explicitValues)
                    if (ev.ccNumber == cc)
                        return ev.value;

                return processor.getCombiResolvedCcValue (presetId, cc);
            });

            if (processor.isUserCombiPresetId (presetId))
            {
                editingId = presetId;
                updateButton.setEnabled (true);
                nameEditor.setText (processor.getCombiPresetLabel (presetId), juce::dontSendNotification);
                statusLabel.setText ("Loaded [User] " + processor.getCombiPresetLabel (presetId)
                                         + " - edit and Update, or Save as New.",
                                     juce::dontSendNotification);
            }
            else
            {
                editingId = -1;
                updateButton.setEnabled (false);
                statusLabel.setText ("Seeded from " + processor.getCombiPresetLabel (presetId)
                                         + " (factory - Save as New).",
                                     juce::dontSendNotification);
            }
        }

        void saveAs (int existingId)
        {
            std::vector<orchconductor::PresetValue> values;
            values.reserve (static_cast<size_t> (sliders.size()));

            for (int i = 0; i < sliders.size(); ++i)
            {
                orchconductor::PresetValue v;
                v.ccNumber = processor.getInstrumentSlotCc (i);
                v.value = juce::roundToInt (sliders[i]->getValue());
                values.push_back (v);
            }

            const int id = processor.saveInstrumentGridAsUserCombi (nameEditor.getText().trim(), values, existingId);

            if (id < 0)
            {
                statusLabel.setText ("Save failed - no free user-combi slot.", juce::dontSendNotification);
                return;
            }

            editingId = id;
            updateButton.setEnabled (true);
            statusLabel.setText ((existingId >= 0 ? juce::String ("Updated ") : juce::String ("Saved new combi: "))
                                     + processor.getCombiPresetLabel (id),
                                 juce::dontSendNotification);

            refreshLoadBox();
            loadBox.setSelectedId (id + 2, juce::dontSendNotification);

            if (onSaved)
                onSaved();
        }

        OrchConductorAudioProcessor& processor;
        int editingId { -1 };

        juce::Label titleLabel, hintLabel, loadLabel, statusLabel;
        juce::TextButton backButton, saveNewButton, updateButton, seedButton, randomButton, clearButton;
        juce::TextEditor nameEditor;
        juce::ComboBox loadBox;

        juce::Viewport viewport;
        juce::Component content;
        juce::OwnedArray<juce::Slider> sliders;
        juce::OwnedArray<juce::Label> rowLabels;
        juce::OwnedArray<juce::Label> sectionHeaders;
        juce::Array<int> sectionHeaderBeforeSlot;
    };

    // The "Narrative Lane" tab: a lane is a sequence of combi "stops" along a
    // 0..1 timeline. Edit stops directly, or "Propose" a whole arc.
    class LaneMakerComponent final : public juce::Component
    {
    public:
        std::function<void()> onBack;
        std::function<void (juce::String laneId)> onLaneActive;   // save or load -> point the Conductor at this lane

        explicit LaneMakerComponent (OrchConductorAudioProcessor& proc)
            : processor (proc)
        {
            setOpaque (true);

            auto styleButton = [] (juce::TextButton& b, juce::Colour c)
            {
                b.setColour (juce::TextButton::buttonColourId, c);
                b.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
            };
            auto styleBox = [] (juce::ComboBox& b)
            {
                b.setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
                b.setColour (juce::ComboBox::textColourId, juce::Colours::white);
                b.setColour (juce::ComboBox::outlineColourId, juce::Colour::fromRGB (70, 85, 95));
            };
            auto styleEditor = [] (juce::TextEditor& e)
            {
                e.setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
                e.setColour (juce::TextEditor::textColourId, juce::Colours::white);
                e.setColour (juce::TextEditor::outlineColourId, juce::Colour::fromRGB (70, 85, 95));
            };

            titleLabel.setText ("Narrative Lane Maker", juce::dontSendNotification);
            titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
            addAndMakeVisible (titleLabel);

            hintLabel.setText ("A lane = combi 'stops' along the 0->1 Narrative Position timeline. Saved to the lane library.",
                               juce::dontSendNotification);
            hintLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (160, 175, 190));
            hintLabel.setFont (juce::FontOptions (12.0f));
            addAndMakeVisible (hintLabel);

            styleButton (backButton, juce::Colour::fromRGB (55, 60, 70));
            backButton.setButtonText ("< Back to Conductor");
            backButton.onClick = [this] { if (onBack) onBack(); };
            addAndMakeVisible (backButton);

            nameEditor.setText ("My Lane", juce::dontSendNotification);
            nameEditor.setSelectAllWhenFocused (true);
            styleEditor (nameEditor);
            addAndMakeVisible (nameEditor);

            styleButton (saveButton, juce::Colour::fromRGB (45, 75, 95));
            saveButton.setButtonText ("Save to Lane Library");
            saveButton.onClick = [this] { save(); };
            addAndMakeVisible (saveButton);

            styleButton (deleteButton, juce::Colour::fromRGB (95, 45, 45));
            deleteButton.setButtonText ("Delete Lane");
            deleteButton.onClick = [this] { removeLane(); };
            addAndMakeVisible (deleteButton);

            loadLabel.setText ("Load lane:", juce::dontSendNotification);
            loadLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            loadLabel.setFont (juce::FontOptions (13.0f));
            addAndMakeVisible (loadLabel);

            styleBox (loadBox);
            loadBox.onChange = [this] { loadSelected(); };
            addAndMakeVisible (loadBox);

            arcLabel.setText ("Propose arc:", juce::dontSendNotification);
            arcLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            arcLabel.setFont (juce::FontOptions (13.0f));
            addAndMakeVisible (arcLabel);

            styleBox (arcBox);
            arcBox.addItemList (OrchConductorAudioProcessor::getNarrativeArcShapeNames(), 1);
            arcBox.setSelectedId (1, juce::dontSendNotification);
            addAndMakeVisible (arcBox);

            stopsLabel.setText ("Stops:", juce::dontSendNotification);
            stopsLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            stopsLabel.setFont (juce::FontOptions (13.0f));
            addAndMakeVisible (stopsLabel);

            stopsSlider.setSliderStyle (juce::Slider::IncDecButtons);
            stopsSlider.setRange (2.0, 16.0, 1.0);
            stopsSlider.setValue (6.0, juce::dontSendNotification);
            stopsSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 44, 22);
            stopsSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
            stopsSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB (28, 36, 46));
            stopsSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB (70, 85, 95));
            stopsSlider.onValueChange = [this] { resizeModelTo (juce::roundToInt (stopsSlider.getValue())); };
            addAndMakeVisible (stopsSlider);

            restlessLabel.setText ("Restlessness:", juce::dontSendNotification);
            restlessLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            restlessLabel.setFont (juce::FontOptions (13.0f));
            addAndMakeVisible (restlessLabel);

            restlessSlider.setSliderStyle (juce::Slider::LinearHorizontal);
            restlessSlider.setRange (0.0, 1.0, 0.01);
            restlessSlider.setValue (0.4, juce::dontSendNotification);
            restlessSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 22);
            restlessSlider.setColour (juce::Slider::trackColourId, juce::Colour::fromRGB (95, 200, 245));
            restlessSlider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
            restlessSlider.setColour (juce::Slider::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
            restlessSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
            restlessSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB (28, 36, 46));
            restlessSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB (70, 85, 95));
            addAndMakeVisible (restlessSlider);

            styleButton (proposeButton, juce::Colour::fromRGB (70, 55, 85));
            proposeButton.setButtonText ("Propose");
            proposeButton.onClick = [this] { propose(); };
            addAndMakeVisible (proposeButton);

            statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (245, 195, 90));
            statusLabel.setFont (juce::FontOptions (12.0f));
            addAndMakeVisible (statusLabel);

            headerRowLabel.setText (" #   Position   Combi", juce::dontSendNotification);
            headerRowLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (120, 210, 250));
            headerRowLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
            addAndMakeVisible (headerRowLabel);

            viewport.setViewedComponent (&content, false);
            viewport.setScrollBarsShown (true, false);
            addAndMakeVisible (viewport);

            resizeModelTo (6);
            refreshLoadBox();
        }

        void visibilityChanged() override
        {
            if (isVisible())
            {
                rebuildCombiChoices();
                rebuildRows();
                refreshLoadBox();   // preserves currentLaneId
            }
        }

        void paint (juce::Graphics& g) override { g.fillAll (juce::Colour::fromRGB (22, 30, 38)); }

        void resized() override
        {
            auto r = getLocalBounds().reduced (18, 12);

            auto headerRow = r.removeFromTop (30);
            backButton.setBounds (headerRow.removeFromLeft (170));
            headerRow.removeFromLeft (16);
            titleLabel.setBounds (headerRow);

            hintLabel.setBounds (r.removeFromTop (18));
            r.removeFromTop (8);

            auto nameRow = r.removeFromTop (30);
            nameEditor.setBounds (nameRow.removeFromLeft (220).reduced (0, 2));
            nameRow.removeFromLeft (8);
            saveButton.setBounds (nameRow.removeFromLeft (180).reduced (0, 2));
            nameRow.removeFromLeft (8);
            deleteButton.setBounds (nameRow.removeFromLeft (130).reduced (0, 2));
            nameRow.removeFromLeft (16);
            loadLabel.setBounds (nameRow.removeFromLeft (74));
            loadBox.setBounds (nameRow.removeFromLeft (240).reduced (0, 2));

            r.removeFromTop (8);

            auto genRow = r.removeFromTop (30);
            arcLabel.setBounds (genRow.removeFromLeft (86));
            arcBox.setBounds (genRow.removeFromLeft (220).reduced (0, 2));
            genRow.removeFromLeft (14);
            stopsLabel.setBounds (genRow.removeFromLeft (48));
            stopsSlider.setBounds (genRow.removeFromLeft (110).reduced (0, 2));
            genRow.removeFromLeft (14);
            restlessLabel.setBounds (genRow.removeFromLeft (86));
            restlessSlider.setBounds (genRow.removeFromLeft (200).reduced (0, 2));
            genRow.removeFromLeft (10);
            proposeButton.setBounds (genRow.removeFromLeft (100).reduced (0, 2));

            r.removeFromTop (6);
            statusLabel.setBounds (r.removeFromTop (18));
            r.removeFromTop (4);
            headerRowLabel.setBounds (r.removeFromTop (18));
            r.removeFromTop (2);

            viewport.setBounds (r);
            layoutRows();
        }

        void layoutRows()
        {
            const int rowH = 30;
            const int width = juce::jmax (560, viewport.getWidth() - 16);

            for (int i = 0; i < rowIndexLabels.size(); ++i)
            {
                const int y = i * rowH;
                auto row = juce::Rectangle<int> (0, y, width, rowH).reduced (2, 3);
                rowIndexLabels[i]->setBounds (row.removeFromLeft (34));
                rowPositionEditors[i]->setBounds (row.removeFromLeft (66).reduced (0, 1));
                row.removeFromLeft (8);
                rowUpButtons[i]->setBounds (row.removeFromRight (28));
                rowDownButtons[i]->setBounds (row.removeFromRight (28));
                rowRemoveButtons[i]->setBounds (row.removeFromRight (28));
                row.removeFromRight (6);
                rowCombiBoxes[i]->setBounds (row.reduced (0, 1));
            }

            content.setSize (width, juce::jmax (viewport.getHeight(), rowIndexLabels.size() * rowH + 6));
        }

    private:
        static juce::String laneIdFromName (const juce::String& name)
        {
            auto id = name.toLowerCase().retainCharacters ("abcdefghijklmnopqrstuvwxyz0123456789 -_")
                          .replaceCharacters (" -", "__").trim();
            while (id.contains ("__")) id = id.replace ("__", "_");
            return id.isNotEmpty() ? id : "custom_lane";
        }

        void rebuildCombiChoices()
        {
            combiChoiceIds.clearQuick();
            combiChoiceLabels.clearQuick();

            for (int id = 1; id <= processor.getMaxCombiPresetId(); ++id)
            {
                const auto label = processor.getCombiPresetLabel (id);
                if (label == "Unknown Combi") continue;
                combiChoiceIds.add (id);
                combiChoiceLabels.add ((processor.isUserCombiPresetId (id) ? juce::String ("[User] ") : juce::String()) + label);
            }
        }

        void refreshLoadBox()
        {
            loadBox.clear (juce::dontSendNotification);
            loadBox.addItem ("(new lane)", 1);

            int selectId = 1;

            for (int i = 0; i < processor.getNarrativeLaneCount(); ++i)
            {
                loadBox.addItem (processor.getNarrativeLaneLabel (i), i + 2);

                if (currentLaneId.isNotEmpty() && processor.getNarrativeLaneId (i) == currentLaneId)
                    selectId = i + 2;
            }

            loadBox.setSelectedId (selectId, juce::dontSendNotification);
        }

        void resizeModelTo (int count)
        {
            count = juce::jlimit (2, 16, count);

            if ((int) model.size() > count)
                model.resize ((size_t) count);
            else
                while ((int) model.size() < count)
                {
                    OrchConductorAudioProcessor::NarrativeLanePointEdit p;
                    p.combiId = model.empty() ? 4 /*chamber*/ : model.back().combiId;
                    model.push_back (p);
                }

            respreadPositions();
            rebuildRows();
        }

        void respreadPositions()
        {
            const int n = (int) model.size();
            for (int i = 0; i < n; ++i)
                model[(size_t) i].position = n > 1 ? (double) i / (double) (n - 1) : 0.0;
        }

        void propose()
        {
            const auto shape = static_cast<OrchConductorAudioProcessor::NarrativeArcShape> (arcBox.getSelectedId() - 1);
            model = processor.generateNarrativeLane (shape,
                                                     juce::roundToInt (stopsSlider.getValue()),
                                                     (float) restlessSlider.getValue(),
                                                     juce::Random::getSystemRandom().nextInt64());
            rebuildRows();
            statusLabel.setText ("Proposed a " + arcBox.getText() + " arc - edit any stop, then Save to Lane Library.",
                                 juce::dontSendNotification);
        }

        void loadSelected()
        {
            const int sel = loadBox.getSelectedId();
            if (sel <= 1) return;

            const int laneIndex = sel - 2;
            const int count = processor.getNarrativeLanePointCount (laneIndex);
            if (count <= 0) return;

            model.clear();

            for (int i = 0; i < count; ++i)
            {
                OrchConductorAudioProcessor::NarrativeLanePointEdit p;
                p.position = processor.getNarrativeLanePointPosition (laneIndex, i);
                p.combiId = processor.getNarrativeLanePointCombiId (laneIndex, i);
                p.pitchFieldIndex = processor.getNarrativeLanePointFieldIndex (laneIndex, i);
                p.harpValue = processor.getNarrativeLanePointHarpValueAt (laneIndex, i);
                p.pianoValue = processor.getNarrativeLanePointPianoValueAt (laneIndex, i);
                p.gateResponseMode = processor.getNarrativeLanePointGateResponseModeAt (laneIndex, i);
                p.gateResponseAmount = processor.getNarrativeLanePointGateResponseAmountAt (laneIndex, i);
                model.push_back (p);
            }

            currentLaneId = processor.getNarrativeLaneId (laneIndex);
            nameEditor.setText (processor.getNarrativeLaneLabel (laneIndex), juce::dontSendNotification);
            stopsSlider.setValue ((double) model.size(), juce::dontSendNotification);
            rebuildRows();

            if (onLaneActive) onLaneActive (currentLaneId);

            statusLabel.setText ("Loaded lane '" + currentLaneId
                                 + "' - now the active lane in the Conductor tab. Edit and Save.",
                                 juce::dontSendNotification);
        }

        void save()
        {
            const auto name = nameEditor.getText().trim();
            const auto id = laneIdFromName (name);

            if (processor.saveNarrativeLane (id, name, {}, model))
            {
                currentLaneId = id;
                refreshLoadBox();

                if (onLaneActive) onLaneActive (id);

                statusLabel.setText ("Saved lane '" + id + "' (" + juce::String (model.size())
                                     + " stops) - now the active lane in the Conductor tab.",
                                     juce::dontSendNotification);
            }
            else
            {
                statusLabel.setText ("Save failed - the lane library needs runtime JSON presets enabled and a valid combi at each stop.",
                                     juce::dontSendNotification);
            }
        }

        void removeLane()
        {
            const auto id = laneIdFromName (nameEditor.getText().trim());

            if (processor.deleteNarrativeLane (id))
            {
                if (currentLaneId == id) currentLaneId.clear();
                refreshLoadBox();
                if (onLaneActive) onLaneActive ({});   // let the Conductor re-clamp its lane selection
                statusLabel.setText ("Deleted lane '" + id + "'.", juce::dontSendNotification);
            }
            else
            {
                statusLabel.setText ("No lane '" + id + "' in the library to delete.", juce::dontSendNotification);
            }
        }

        void rebuildRows()
        {
            rowIndexLabels.clear();
            rowPositionEditors.clear();
            rowCombiBoxes.clear();
            rowUpButtons.clear();
            rowDownButtons.clear();
            rowRemoveButtons.clear();

            if (combiChoiceIds.isEmpty())
                rebuildCombiChoices();

            for (int i = 0; i < (int) model.size(); ++i)
            {
                auto* idx = new juce::Label();
                idx->setText (juce::String (i + 1), juce::dontSendNotification);
                idx->setColour (juce::Label::textColourId, juce::Colours::white);
                idx->setFont (juce::FontOptions (13.0f));
                content.addAndMakeVisible (idx);
                rowIndexLabels.add (idx);

                auto* pos = new juce::TextEditor();
                pos->setText (juce::String (model[(size_t) i].position, 3), juce::dontSendNotification);
                pos->setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
                pos->setColour (juce::TextEditor::textColourId, juce::Colours::white);
                pos->setColour (juce::TextEditor::outlineColourId, juce::Colour::fromRGB (70, 85, 95));
                pos->onFocusLost = [this, i] { commitPosition (i); };
                pos->onReturnKey = [this, i] { commitPosition (i); };
                content.addAndMakeVisible (pos);
                rowPositionEditors.add (pos);

                auto* box = new juce::ComboBox();
                box->setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
                box->setColour (juce::ComboBox::textColourId, juce::Colours::white);
                box->setColour (juce::ComboBox::outlineColourId, juce::Colour::fromRGB (70, 85, 95));
                for (int c = 0; c < combiChoiceIds.size(); ++c)
                    box->addItem (combiChoiceLabels[c], combiChoiceIds[c] + 1);
                box->setSelectedId (model[(size_t) i].combiId + 1, juce::dontSendNotification);
                box->onChange = [this, i, box] { model[(size_t) i].combiId = box->getSelectedId() - 1; };
                content.addAndMakeVisible (box);
                rowCombiBoxes.add (box);

                auto* up = new juce::TextButton ("^");
                up->onClick = [this, i] { moveRow (i, -1); };
                content.addAndMakeVisible (up);
                rowUpButtons.add (up);

                auto* down = new juce::TextButton ("v");
                down->onClick = [this, i] { moveRow (i, +1); };
                content.addAndMakeVisible (down);
                rowDownButtons.add (down);

                auto* rm = new juce::TextButton ("X");
                rm->onClick = [this, i] { removeRow (i); };
                content.addAndMakeVisible (rm);
                rowRemoveButtons.add (rm);
            }

            resized();
        }

        void commitPosition (int i)
        {
            if (i < 0 || i >= (int) model.size()) return;
            model[(size_t) i].position = juce::jlimit (0.0, 1.0, rowPositionEditors[i]->getText().getDoubleValue());
        }

        void moveRow (int i, int dir)
        {
            const int j = i + dir;
            if (i < 0 || j < 0 || i >= (int) model.size() || j >= (int) model.size()) return;
            std::swap (model[(size_t) i].combiId, model[(size_t) j].combiId);
            std::swap (model[(size_t) i].pitchFieldIndex, model[(size_t) j].pitchFieldIndex);
            std::swap (model[(size_t) i].harpValue, model[(size_t) j].harpValue);
            std::swap (model[(size_t) i].pianoValue, model[(size_t) j].pianoValue);
            std::swap (model[(size_t) i].gateResponseMode, model[(size_t) j].gateResponseMode);
            std::swap (model[(size_t) i].gateResponseAmount, model[(size_t) j].gateResponseAmount);
            rebuildRows();
        }

        void removeRow (int i)
        {
            if ((int) model.size() <= 2 || i < 0 || i >= (int) model.size()) return;
            model.erase (model.begin() + i);
            respreadPositions();
            stopsSlider.setValue ((double) model.size(), juce::dontSendNotification);
            rebuildRows();
        }

        OrchConductorAudioProcessor& processor;
        std::vector<OrchConductorAudioProcessor::NarrativeLanePointEdit> model;
        juce::String currentLaneId;

        juce::Label titleLabel, hintLabel, loadLabel, arcLabel, stopsLabel, restlessLabel, statusLabel, headerRowLabel;
        juce::TextButton backButton, saveButton, deleteButton, proposeButton;
        juce::TextEditor nameEditor;
        juce::ComboBox loadBox, arcBox;
        juce::Slider stopsSlider, restlessSlider;

        juce::Viewport viewport;
        juce::Component content;
        juce::OwnedArray<juce::Label> rowIndexLabels;
        juce::OwnedArray<juce::TextEditor> rowPositionEditors;
        juce::OwnedArray<juce::ComboBox> rowCombiBoxes;
        juce::OwnedArray<juce::TextButton> rowUpButtons, rowDownButtons, rowRemoveButtons;
        juce::Array<int> combiChoiceIds;
        juce::StringArray combiChoiceLabels;
    };
}

OrchConductorAudioProcessorEditor::OrchConductorAudioProcessorEditor (OrchConductorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizable (true, true);
    setResizeLimits (860, 480, 1400, 1300);
    setSize (980, 760);

    titleLabel.setText ("OrchConductor", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Orchestration Preset Sender", juce::dontSendNotification);
    subtitleLabel.setJustificationType (juce::Justification::centred);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    subtitleLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (subtitleLabel);

    // orchConductorBuildTimestamp is regenerated on every single build (see
    // cmake/GenerateOrchConductorBuildInfo.cmake) - a hand-maintained phase
    // tag here can't answer "is this actually the build I just installed",
    // a fresh timestamp always can.
    buildLabel.setText (juce::String ("Build: ") + orchConductorBuildTimestamp, juce::dontSendNotification);
    buildLabel.setJustificationType (juce::Justification::centred);
    buildLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (140, 160, 180));
    buildLabel.setFont (juce::FontOptions (10.5f));
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

    auto setupManualCcSlider = [this] (juce::Slider& slider, juce::Label& label,
                                       const juce::String& text, int initialValue)
    {
        label.setText (text, juce::dontSendNotification);
        styleLabel (label, juce::Colours::white, 14.0f, juce::Font::bold);
        addAndMakeVisible (label);

        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 22);
        slider.setRange (-1.0, 127.0, 1.0);
        slider.setValue ((double) initialValue, juce::dontSendNotification);
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

    setupManualCcSlider (userCombiHarpSlider, userCombiHarpLabel, "Harp",
                         audioProcessor.getManualHarpValue());
    setupManualCcSlider (userCombiPianoSlider, userCombiPianoLabel, "Piano",
                         audioProcessor.getManualPianoValue());

    userCombiHarpSlider.onValueChange = [this]
    {
        audioProcessor.setManualHarpValue (juce::roundToInt (userCombiHarpSlider.getValue()));
        updateOutputTable();
        updateStatus();
    };

    userCombiPianoSlider.onValueChange = [this]
    {
        audioProcessor.setManualPianoValue (juce::roundToInt (userCombiPianoSlider.getValue()));
        updateOutputTable();
        updateStatus();
    };

    // --- Live Gate Response panel -----------------------------------------
    styleLabel (gateResponseLabel, juce::Colours::white, 14.0f, juce::Font::bold);
    gateResponseLabel.setText ("Gate Response Bridge (CC106/107 -> OrchGate)", juce::dontSendNotification);
    addAndMakeVisible (gateResponseLabel);

    gateResponseEnableButton.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    gateResponseEnableButton.setToggleState (audioProcessor.isGateResponseManualEnabled(), juce::dontSendNotification);
    gateResponseEnableButton.onClick = [this]
    {
        audioProcessor.setGateResponseManualEnabled (gateResponseEnableButton.getToggleState());
        updateStatus();
    };
    addAndMakeVisible (gateResponseEnableButton);

    auto styleGateSlider = [this] (juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        styleLabel (l, juce::Colours::white, 13.0f, juce::Font::plain);
        l.setText (text, juce::dontSendNotification);
        addAndMakeVisible (l);

        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 22);
        s.setRange (0.0, 127.0, 1.0);
        s.setColour (juce::Slider::backgroundColourId, juce::Colour::fromRGB (28, 36, 46));
        s.setColour (juce::Slider::trackColourId, juce::Colour::fromRGB (95, 200, 245));
        s.setColour (juce::Slider::thumbColourId, juce::Colours::white);
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB (28, 36, 46));
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB (70, 85, 95));
        addAndMakeVisible (s);
    };

    styleGateSlider (gateResponseAmountSlider, gateResponseAmountLabel, "Amount");
    styleGateSlider (gateResponseModeSlider, gateResponseModeLabel, "Mode");
    gateResponseAmountSlider.setValue (audioProcessor.getGateResponseManualAmount(), juce::dontSendNotification);
    gateResponseModeSlider.setValue (audioProcessor.getGateResponseManualMode(), juce::dontSendNotification);

    gateResponseAmountSlider.onValueChange = [this]
    {
        audioProcessor.setGateResponseManualAmount (juce::roundToInt (gateResponseAmountSlider.getValue()));
        updateStatus();
    };
    gateResponseModeSlider.onValueChange = [this]
    {
        audioProcessor.setGateResponseManualMode (juce::roundToInt (gateResponseModeSlider.getValue()));
        updateStatus();
    };

    gateResponseShuffleButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (45, 75, 95));
    gateResponseShuffleButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    gateResponseShuffleButton.onClick = [this]
    {
        audioProcessor.shuffleGateResponseMode();
        gateResponseEnableButton.setToggleState (true, juce::dontSendNotification);
        gateResponseModeSlider.setValue (audioProcessor.getGateResponseManualMode(), juce::dontSendNotification);
        updateStatus();
    };
    addAndMakeVisible (gateResponseShuffleButton);

    styleLabel (gateResponseStatusLabel, juce::Colour::fromRGB (140, 200, 245), 12.0f, juce::Font::plain);
    gateResponseStatusLabel.setText ("Bridge idle", juce::dontSendNotification);
    addAndMakeVisible (gateResponseStatusLabel);

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

        const int harpValue = audioProcessor.getManualHarpValue();
        const int pianoValue = audioProcessor.getManualPianoValue();

        const auto id = audioProcessor.createUserCombiPresetFromCurrentSections (name);

        if (id >= 0)
        {
            addCombiPresetItems (combiPresetBox, audioProcessor);
            combiPresetBox.setSelectedId (id + 1, juce::sendNotificationSync);

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
        "CC49 Harp / CC55 Piano: manual sliders (Manual Sections mode), or per-combi / per-lane-point overrides",
        juce::dontSendNotification);
    ccMapLabel.setJustificationType (juce::Justification::centred);
    ccMapLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (160, 175, 190));
    ccMapLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (ccMapLabel);

    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (245, 195, 90));
    statusLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (statusLabel);

    // View switch + the Combi Grid / Narrative Lane tabs.
    for (auto* b : { &conductorViewButton, &gridViewButton, &laneViewButton })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (40, 52, 64));
        b->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible (*b);
    }
    conductorViewButton.onClick = [this] { showView (View::conductor); };
    gridViewButton.onClick      = [this] { showView (View::grid); };
    laneViewButton.onClick      = [this] { showView (View::lane); };

    {
        auto grid = std::make_unique<InstrumentGridComponent> (audioProcessor);
        grid->onBack = [this] { showView (View::conductor); };
        grid->onSaved = [this]
        {
            addCombiPresetItems (combiPresetBox, audioProcessor);
            updateStatus();
        };
        addChildComponent (*grid);
        instrumentGridView = std::move (grid);
    }

    {
        auto lm = std::make_unique<LaneMakerComponent> (audioProcessor);
        lm->onBack = [this] { showView (View::conductor); };
        lm->onLaneActive = [this] (juce::String laneId)
        {
            rebuildNarrativeLaneItems();

            int idx = -1;
            for (int i = 0; i < audioProcessor.getNarrativeLaneCount(); ++i)
                if (audioProcessor.getNarrativeLaneId (i) == laneId)
                    idx = i;

            if (idx >= 0)
            {
                audioProcessor.setNarrativeLaneIndex (idx);
                audioProcessor.requestNarrativeReresolve();
            }

            narrativeLaneBox.setSelectedId (audioProcessor.getNarrativeLaneIndex() + 1, juce::dontSendNotification);
            updateNarrativeScanControls();
            updateStatus();
        };
        addChildComponent (*lm);
        laneMakerView = std::move (lm);
    }

    // Move every Conductor control into a scroll viewport so the window can be
    // made shorter than the content. Pinned outside it: the view-switch
    // buttons, the grid / lane tabs, and the two footer status lines.
    {
        conductorContent = std::make_unique<WheelForwardingComponent>();

        juce::Array<juce::Component*> pinned { &conductorViewButton, &gridViewButton, &laneViewButton,
                                              &statusLabel, &ccMapLabel,
                                              instrumentGridView.get(), laneMakerView.get() };

        const juce::Array<juce::Component*> currentChildren (getChildren());

        for (auto* child : currentChildren)
            if (! pinned.contains (child))
                conductorContent->addAndMakeVisible (*child);   // reparents

        // Content intercepts so its mouseWheelMove (wheel forwarding) runs;
        // children still receive their own events.
        conductorContent->setInterceptsMouseClicks (true, true);
        conductorViewport.setViewedComponent (conductorContent.get(), false);
        conductorViewport.setScrollBarsShown (true, false);
        conductorViewport.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::nonHover);
        addAndMakeVisible (conductorViewport);
        conductorViewport.toBack();
    }

    updateOutputTable();
    updateWoodwindsOutputTable();
    updateBrassOutputTable();
    updatePercussionOutputTable();
    updateNarrativeMetadataDisplay();
    updateNarrativeScanControls();
    updateStatus();
    showView (View::conductor);
    resized(); // lay out the tab children now that they exist

    // Phase 2A: keep UI synced when host restores plugin state after editor creation.
    startTimerHz (10);
}

void OrchConductorAudioProcessorEditor::showView (View view)
{
    const bool conductor = view == View::conductor;

    conductorViewport.setVisible (conductor);
    statusLabel.setVisible (conductor);
    ccMapLabel.setVisible (conductor);

    if (instrumentGridView != nullptr)
    {
        instrumentGridView->setVisible (view == View::grid);
        if (view == View::grid) instrumentGridView->toFront (false);
    }

    if (laneMakerView != nullptr)
    {
        laneMakerView->setVisible (view == View::lane);
        if (view == View::lane) laneMakerView->toFront (false);
    }

    conductorViewButton.setEnabled (! conductor);
    gridViewButton.setEnabled (view != View::grid);
    laneViewButton.setEnabled (view != View::lane);

    for (auto* b : { &conductorViewButton, &gridViewButton, &laneViewButton })
        b->toFront (false);
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
    {
        auto strip = getLocalBounds().reduced (18, 0).removeFromTop (30).withTrimmedTop (5);
        conductorViewButton.setBounds (strip.removeFromLeft (130));
        strip.removeFromLeft (6);
        gridViewButton.setBounds (strip.removeFromLeft (130));
        strip.removeFromLeft (6);
        laneViewButton.setBounds (strip.removeFromLeft (150));
    }

    // Footer, pinned to the window bottom (not scrolled).
    auto footer = getLocalBounds().reduced (48, 0);
    footer.removeFromBottom (12);
    statusLabel.setBounds (footer.removeFromBottom (24));
    footer.removeFromBottom (3);
    ccMapLabel.setBounds (footer.removeFromBottom (20));
    const int footerHeight = 12 + 24 + 3 + 20;

    if (instrumentGridView != nullptr)
        instrumentGridView->setBounds (getLocalBounds().withTrimmedTop (32));

    if (laneMakerView != nullptr)
        laneMakerView->setBounds (getLocalBounds().withTrimmedTop (32));

    conductorViewport.setBounds (getLocalBounds().withTrimmedTop (32).withTrimmedBottom (footerHeight));

    // Reserve room for the vertical scrollbar - the Conductor content is always
    // taller than a comfortable window.
    const int contentW = juce::jmax (840, conductorViewport.getWidth() - 14);

    auto area = juce::Rectangle<int> (0, 0, contentW, 5000).reduced (48, 10);

    titleLabel.setBounds (area.removeFromTop (28));
    subtitleLabel.setBounds (area.removeFromTop (16));
    buildLabel.setBounds (area.removeFromTop (14));

    area.removeFromTop (10);

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

    area.removeFromTop (8);

    // Harp / Piano: a manual control surface in Manual Sections mode, folded
    // into "Save Current Sections as Combi". Same two-column layout as the
    // section rows above.
    auto sectionRow3 = area.removeFromTop (34);
    auto leftHp = sectionRow3.removeFromLeft (424);
    sectionRow3.removeFromLeft (36);
    auto rightHp = sectionRow3.removeFromLeft (424);

    userCombiHarpLabel.setBounds (leftHp.removeFromLeft (110));
    userCombiHarpSlider.setBounds (leftHp);

    userCombiPianoLabel.setBounds (rightHp.removeFromLeft (110));
    userCombiPianoSlider.setBounds (rightHp);

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

    gateResponseLabel.setBounds (area.removeFromTop (22));
    area.removeFromTop (2);
    {
        auto row = area.removeFromTop (30);
        gateResponseEnableButton.setBounds (row.removeFromLeft (230));
        row.removeFromLeft (12);
        gateResponseShuffleButton.setBounds (row.removeFromLeft (110).reduced (0, 2));
    }
    {
        auto row = area.removeFromTop (30);
        gateResponseAmountLabel.setBounds (row.removeFromLeft (64));
        gateResponseAmountSlider.setBounds (row.removeFromLeft (360));
        row.removeFromLeft (20);
        gateResponseModeLabel.setBounds (row.removeFromLeft (54));
        gateResponseModeSlider.setBounds (row.removeFromLeft (300));
    }
    gateResponseStatusLabel.setBounds (area.removeFromTop (18));

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

    if (conductorContent != nullptr)
        conductorContent->setSize (contentW, area.getY() + 16);
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

    if (juce::roundToInt (userCombiHarpSlider.getValue()) != audioProcessor.getManualHarpValue())
        userCombiHarpSlider.setValue (audioProcessor.getManualHarpValue(), juce::dontSendNotification);

    if (juce::roundToInt (userCombiPianoSlider.getValue()) != audioProcessor.getManualPianoValue())
        userCombiPianoSlider.setValue (audioProcessor.getManualPianoValue(), juce::dontSendNotification);

    if (gateResponseEnableButton.getToggleState() != audioProcessor.isGateResponseManualEnabled())
        gateResponseEnableButton.setToggleState (audioProcessor.isGateResponseManualEnabled(), juce::dontSendNotification);

    if (juce::roundToInt (gateResponseAmountSlider.getValue()) != audioProcessor.getGateResponseManualAmount())
        gateResponseAmountSlider.setValue (audioProcessor.getGateResponseManualAmount(), juce::dontSendNotification);

    if (juce::roundToInt (gateResponseModeSlider.getValue()) != audioProcessor.getGateResponseManualMode())
        gateResponseModeSlider.setValue (audioProcessor.getGateResponseManualMode(), juce::dontSendNotification);

    {
        const int effMode = audioProcessor.getEffectiveGateResponseMode();
        const int effAmount = audioProcessor.getEffectiveGateResponseAmount();

        juce::String gr;
        if (effMode < 0 && effAmount < 0)
            gr = audioProcessor.isGateResponseManualEnabled() ? "Bridge on - mode 0" : "Bridge idle (OrchGate on own knobs)";
        else
            gr = "Broadcasting  CC106 = " + juce::String (juce::jmax (0, effMode))
               + "   CC107 = " + juce::String (juce::jmax (0, effAmount))
               + (audioProcessor.isGateResponseManualEnabled() ? "  (panel)" : "  (lane point)");

        if (gateResponseStatusLabel.getText() != gr)
            gateResponseStatusLabel.setText (gr, juce::dontSendNotification);
    }

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
        + " | CC49 Harp " + juce::String (audioProcessor.getHarpCcValue())
        + " / CC55 Piano " + juce::String (audioProcessor.getPianoCcValue()),
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
            + " | Harp " + juce::String (audioProcessor.getHarpCcValue())
            + " / Piano " + juce::String (audioProcessor.getPianoCcValue())
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
        + " | Harp " + juce::String (audioProcessor.getHarpCcValue())
        + " / Piano " + juce::String (audioProcessor.getPianoCcValue())
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

    text << "\n";
    appendMidiMapSectionHeader (text, "Control plane (broadcast, not per-instrument)");
    text << "CC102 Narrative Position   CC103 Narrative Lane   CC104 Authority Mode\n";
    text << "CC105 Field Select (OrchNoteFilter pitch-class field)\n";
    {
        const int m = audioProcessor.getEffectiveGateResponseMode();
        const int a = audioProcessor.getEffectiveGateResponseAmount();
        text << "CC106 Gate Response Mode   = " << (m < 0 ? juce::String ("(idle)") : juce::String (m))
             << "   CC107 Gate Response Amount = " << (a < 0 ? juce::String ("(idle)") : juce::String (a))
             << "\n      (every OrchGate set to \"Follow Conductor Response\" randomises its own\n"
             << "       CC Invert / Threshold / participation from these)\n";
    }

    text << "\nUse the per-instrument CC numbers above as the assigned CC Gate value in each\n";
    text << "OrchGate instance. CC106/107 are a single broadcast the whole rig follows.";

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


