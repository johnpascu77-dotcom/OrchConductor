#include "OrchConductorProcessor.h"
#include "OrchConductorEditor.h"

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

    struct CcValue
    {
        int cc;
        int value;
    };

    // Phase 1A.1 fixed CC map:
    // CC20 Violin I
    // CC21 Violin II
    // CC22 Viola
    // CC23 Cello
    // CC24 Double Bass
    CcValue values[] =
    {
        { 20, 0 },
        { 21, 0 },
        { 22, 0 },
        { 23, 0 },
        { 24, 0 }
    };

    if (! shouldSendAllOff)
    {
        switch (currentPreset)
        {
            case Preset::allOff:
                break;

            case Preset::stringQuartet:
                values[0].value = 127;
                values[1].value = 127;
                values[2].value = 127;
                values[3].value = 127;
                values[4].value = 0;
                break;

            case Preset::lowStrings:
                values[0].value = 0;
                values[1].value = 0;
                values[2].value = 64;
                values[3].value = 127;
                values[4].value = 127;
                break;

            case Preset::fullStrings:
                values[0].value = 127;
                values[1].value = 127;
                values[2].value = 127;
                values[3].value = 127;
                values[4].value = 127;
                break;

            case Preset::tutti:
                values[0].value = 127;
                values[1].value = 127;
                values[2].value = 127;
                values[3].value = 127;
                values[4].value = 127;
                break;
        }
    }

    for (const auto& v : values)
        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, v.cc, v.value), 0);
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrchConductorAudioProcessor();
}
