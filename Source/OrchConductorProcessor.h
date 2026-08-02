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

    enum class Preset
    {
        allOff = 0,
        stringQuartet,
        lowStrings,
        fullStrings,
        tutti
    };

    void setPreset (Preset newPreset);
    Preset getPreset() const;

    void requestSendPreset();
    bool consumeSendPresetRequest();

    juce::String getPresetName() const;

private:
    Preset currentPreset { Preset::allOff };
    bool sendPresetRequested { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchConductorAudioProcessor)
};
