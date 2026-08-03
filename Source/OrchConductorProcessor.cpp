#include "OrchConductorProcessor.h"
#include "OrchConductorEditor.h"

namespace
{
    constexpr int numRows = 5;

    const char* instrumentNames[numRows] =
    {
        "Violin I",
        "Violin II",
        "Viola",
        "Cello",
        "Double Bass"
    };

    constexpr int ccNumbers[numRows] =
    {
        20, 21, 22, 23, 24
    };
}

OrchConductorAudioProcessor::OrchConductorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties())
#endif
{
}

OrchConductorAudioProcessor::~OrchConductorAudioProcessor()
{
}

const juce::String OrchConductorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OrchConductorAudioProcessor::acceptsMidi() const
{
    return true;
}

bool OrchConductorAudioProcessor::producesMidi() const
{
    return true;
}

bool OrchConductorAudioProcessor::isMidiEffect() const
{
    return true;
}

double OrchConductorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OrchConductorAudioProcessor::getNumPrograms()
{
    return 1;
}

int OrchConductorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OrchConductorAudioProcessor::setCurrentProgram (int)
{
}

const juce::String OrchConductorAudioProcessor::getProgramName (int)
{
    return {};
}

void OrchConductorAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void OrchConductorAudioProcessor::prepareToPlay (double, int)
{
}

void OrchConductorAudioProcessor::releaseResources()
{
}

bool OrchConductorAudioProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
    return true;
}

void OrchConductorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    const bool shouldSendAllOff = consumeSendAllOffRequest();
    const bool shouldSendPreset = consumeSendPresetRequest();

    if (! shouldSendAllOff && ! shouldSendPreset)
        return;

    for (int i = 0; i < numRows; ++i)
    {
        const int value = shouldSendAllOff ? 0 : getPresetValueForIndex (i);
        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, ccNumbers[i], value), 0);
    }
}

bool OrchConductorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OrchConductorAudioProcessor::createEditor()
{
    return new OrchConductorAudioProcessorEditor (*this);
}

void OrchConductorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, true);
    stream.writeInt (static_cast<int> (currentPreset));
    stream.writeBool (sendOnPresetChange);
}

void OrchConductorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);
    const auto p = stream.readInt();

    if (p >= 0 && p <= static_cast<int> (Preset::tutti))
        currentPreset = static_cast<Preset> (p);

    if (! stream.isExhausted())
        sendOnPresetChange = stream.readBool();
}

void OrchConductorAudioProcessor::setPreset (Preset newPreset)
{
    currentPreset = newPreset;

    if (sendOnPresetChange)
        requestSendPreset();
}

OrchConductorAudioProcessor::Preset OrchConductorAudioProcessor::getPreset() const
{
    return currentPreset;
}

void OrchConductorAudioProcessor::requestSendPreset()
{
    sendPresetRequested = true;
}

void OrchConductorAudioProcessor::requestSendAllOff()
{
    sendAllOffRequested = true;
}

bool OrchConductorAudioProcessor::consumeSendPresetRequest()
{
    if (! sendPresetRequested)
        return false;

    sendPresetRequested = false;
    return true;
}

bool OrchConductorAudioProcessor::consumeSendAllOffRequest()
{
    if (! sendAllOffRequested)
        return false;

    sendAllOffRequested = false;
    return true;
}

void OrchConductorAudioProcessor::setSendOnPresetChange (bool shouldSend)
{
    sendOnPresetChange = shouldSend;
}

bool OrchConductorAudioProcessor::getSendOnPresetChange() const
{
    return sendOnPresetChange;
}

juce::String OrchConductorAudioProcessor::getPresetName() const
{
    switch (currentPreset)
    {
        case Preset::allOff:        return "All Off";
        case Preset::stringQuartet: return "String Quartet";
        case Preset::lowStrings:    return "Low Strings";
        case Preset::fullStrings:   return "Full Strings";
        case Preset::tutti:         return "Tutti";
    }

    return "Unknown";
}

int OrchConductorAudioProcessor::getNumOutputRows()
{
    return numRows;
}

OrchConductorAudioProcessor::OutputRow OrchConductorAudioProcessor::getOutputRow (int index) const
{
    if (index < 0 || index >= numRows)
        return { "Invalid", 0, 0 };

    return
    {
        instrumentNames[index],
        ccNumbers[index],
        getPresetValueForIndex (index)
    };
}

int OrchConductorAudioProcessor::getPresetValueForIndex (int index) const
{
    if (index < 0 || index >= numRows)
        return 0;

    switch (currentPreset)
    {
        case Preset::allOff:
            return 0;

        case Preset::stringQuartet:
            // Violin I, Violin II, Viola, Cello on. Double Bass off.
            return index <= 3 ? 127 : 0;

        case Preset::lowStrings:
            // Viola half, Cello full, Double Bass full.
            if (index == 2) return 64;
            if (index == 3) return 127;
            if (index == 4) return 127;
            return 0;

        case Preset::fullStrings:
            return 127;

        case Preset::tutti:
            return 127;
    }

    return 0;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrchConductorAudioProcessor();
}
