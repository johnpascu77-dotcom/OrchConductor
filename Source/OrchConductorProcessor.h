#pragma once

#include <JuceHeader.h>

class OrchConductorAudioProcessor  : public juce::AudioProcessor
{
public:
    OrchConductorAudioProcessor();
    ~OrchConductorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    enum class Section
    {
        woodwinds = 0,
        brass,
        percussion,
        strings
    };

    enum class CombiPreset
    {
        manualSections = 0
    };

    enum class Preset
    {
        allOff = 0,
        violinIOnly,
        violinIIOnly,
        violinsOnly,
        violasOnly,
        cellosOnly,
        bassesOnly,
        upperStrings,
        lowStrings,
        stringQuartet,
        violaCello,
        celloBass,
        fullStrings,
        tutti
    };

    struct OutputRow
    {
        juce::String instrumentName;
        int ccNumber;
        int value;
    };

    int getCombiPresetId() const;
    void setCombiPresetId (int presetId);

    int getSectionPresetId (Section section) const;
    void setSectionPresetId (Section section, int presetId);

    void setPreset (Preset newPreset);
    Preset getPreset() const;

    void requestSendPreset();
    void requestSendAllOff();

    bool consumeSendPresetRequest();
    bool consumeSendAllOffRequest();

    void setSendOnPresetChange (bool shouldSend);
    bool getSendOnPresetChange() const;

    juce::String getPresetName() const;

    static int getNumOutputRows();
    OutputRow getOutputRow (int index) const;

    static int getNumWoodwindsOutputRows();
    OutputRow getWoodwindsOutputRow (int index) const;

private:
    static constexpr int minCombiPresetId = 0;
    static constexpr int maxCombiPresetId = 0;

    static constexpr int minPlaceholderSectionPresetId = 0;
    static constexpr int maxPlaceholderSectionPresetId = 5;

    static constexpr int minStringsPresetId = 0;
    static constexpr int maxStringsPresetId = static_cast<int> (Preset::tutti);

    int combiPresetId { static_cast<int> (CombiPreset::manualSections) };

    int woodwindsPresetId { 0 };
    int brassPresetId { 0 };
    int percussionPresetId { 0 };
    int stringsPresetId { static_cast<int> (Preset::allOff) };

    bool sendPresetRequested { false };
    bool sendAllOffRequested { false };
    bool sendOnPresetChange { false };

    int getPresetValueForIndex (int index) const;
    int getWoodwindsPresetValueForIndex (int index) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchConductorAudioProcessor)
};
