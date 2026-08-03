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
        50, 51, 52, 53, 54
    };

    constexpr int numWoodwindsRows = 12;

    const char* woodwindsInstrumentNames[numWoodwindsRows] =
    {
        "Piccolo",
        "Flute 1",
        "Flute 2",
        "Oboe 1",
        "Oboe 2",
        "English Horn",
        "Clarinet 1",
        "Clarinet 2",
        "Bass Clarinet",
        "Bassoon 1",
        "Bassoon 2",
        "Contrabassoon"
    };

    constexpr int woodwindsCcNumbers[numWoodwindsRows] =
    {
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    };

    constexpr int numBrassRows = 11;

    const char* brassInstrumentNames[numBrassRows] =
    {
        "Horn 1",
        "Horn 2",
        "Horn 3",
        "Horn 4",
        "Trumpet 1",
        "Trumpet 2",
        "Trumpet 3",
        "Trombone 1",
        "Trombone 2",
        "Bass Trombone",
        "Tuba"
    };

    constexpr int brassCcNumbers[numBrassRows] =
    {
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42
    };

    constexpr int numPercussionRows = 6;

    const char* percussionInstrumentNames[numPercussionRows] =
    {
        "Timpani",
        "Glockenspiel",
        "Xylophone",
        "Marimba",
        "Vibraphone",
        "Tubular Bells"
    };

    constexpr int percussionCcNumbers[numPercussionRows] =
    {
        43, 44, 45, 46, 47, 48
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

    // Phase 1G: Strings emit in full-score order after reserved Harp CC49.
    for (int i = 0; i < numRows; ++i)
    {
        const int value = shouldSendAllOff ? 0 : getPresetValueForIndex (i);
        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, ccNumbers[i], value), 0);
    }

    // Phase 1G: Woodwinds emit individual full-score track CC output.
    for (int i = 0; i < numWoodwindsRows; ++i)
    {
        const int value = shouldSendAllOff ? 0 : getWoodwindsPresetValueForIndex (i);
        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, woodwindsCcNumbers[i], value), 0);
    }

    // Phase 1G: Brass emits individual full-score track CC output.
    for (int i = 0; i < numBrassRows; ++i)
    {
        const int value = shouldSendAllOff ? 0 : getBrassPresetValueForIndex (i);
        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, brassCcNumbers[i], value), 0);
    }

    // Phase 1G: Melodic percussion emits full-score track CC output.
    for (int i = 0; i < numPercussionRows; ++i)
    {
        const int value = shouldSendAllOff ? 0 : getPercussionPresetValueForIndex (i);
        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, percussionCcNumbers[i], value), 0);
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

    stream.writeInt (1);
    stream.writeInt (combiPresetId);
    stream.writeInt (woodwindsPresetId);
    stream.writeInt (brassPresetId);
    stream.writeInt (percussionPresetId);
    stream.writeInt (stringsPresetId);
    stream.writeBool (sendOnPresetChange);
}

void OrchConductorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);
    const auto firstInt = stream.readInt();

    if (stream.isExhausted())
    {
        if (firstInt >= minStringsPresetId && firstInt <= maxStringsPresetId)
            stringsPresetId = firstInt;

        return;
    }

    if (firstInt == 1)
    {
        const auto combi = stream.readInt();
        const auto woodwinds = stream.readInt();
        const auto brass = stream.readInt();
        const auto percussion = stream.readInt();
        const auto strings = stream.readInt();

        setCombiPresetId (combi);
        setSectionPresetId (Section::woodwinds, woodwinds);
        setSectionPresetId (Section::brass, brass);
        setSectionPresetId (Section::percussion, percussion);
        setSectionPresetId (Section::strings, strings);

        if (! stream.isExhausted())
            sendOnPresetChange = stream.readBool();

        return;
    }

    if (firstInt >= minStringsPresetId && firstInt <= maxStringsPresetId)
        stringsPresetId = firstInt;

    if (! stream.isExhausted())
        sendOnPresetChange = stream.readBool();
}

int OrchConductorAudioProcessor::getCombiPresetId() const
{
    return combiPresetId;
}

void OrchConductorAudioProcessor::setCombiPresetId (int presetId)
{
    if (presetId >= minCombiPresetId && presetId <= maxCombiPresetId)
        combiPresetId = presetId;
}

int OrchConductorAudioProcessor::getSectionPresetId (Section section) const
{
    switch (section)
    {
        case Section::woodwinds:  return woodwindsPresetId;
        case Section::brass:      return brassPresetId;
        case Section::percussion: return percussionPresetId;
        case Section::strings:    return stringsPresetId;
    }

    return 0;
}

void OrchConductorAudioProcessor::setSectionPresetId (Section section, int presetId)
{
    switch (section)
    {
        case Section::woodwinds:
            if (presetId >= minPlaceholderSectionPresetId && presetId <= maxPlaceholderSectionPresetId)
                woodwindsPresetId = presetId;
            break;

        case Section::brass:
            if (presetId >= minPlaceholderSectionPresetId && presetId <= maxPlaceholderSectionPresetId)
                brassPresetId = presetId;
            break;

        case Section::percussion:
            if (presetId >= minPlaceholderSectionPresetId && presetId <= maxPlaceholderSectionPresetId)
                percussionPresetId = presetId;
            break;

        case Section::strings:
            if (presetId >= minStringsPresetId && presetId <= maxStringsPresetId)
            {
                stringsPresetId = presetId;

                if (sendOnPresetChange)
                    requestSendPreset();
            }
            break;
    }
}

void OrchConductorAudioProcessor::setPreset (Preset newPreset)
{
    setSectionPresetId (Section::strings, static_cast<int> (newPreset));
}

OrchConductorAudioProcessor::Preset OrchConductorAudioProcessor::getPreset() const
{
    return static_cast<Preset> (stringsPresetId);
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
    switch (getPreset())
    {
        case Preset::allOff:        return "All Off";
        case Preset::violinIOnly:   return "Violin I Only";
        case Preset::violinIIOnly:  return "Violin II Only";
        case Preset::violinsOnly:   return "Violins Only";
        case Preset::violasOnly:    return "Violas Only";
        case Preset::cellosOnly:    return "Cellos Only";
        case Preset::bassesOnly:    return "Basses Only";
        case Preset::upperStrings:  return "Upper Strings";
        case Preset::lowStrings:    return "Low Strings";
        case Preset::stringQuartet: return "String Quartet";
        case Preset::violaCello:    return "Viola + Cello";
        case Preset::celloBass:     return "Cello + Bass";
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

int OrchConductorAudioProcessor::getNumWoodwindsOutputRows()
{
    return numWoodwindsRows;
}

OrchConductorAudioProcessor::OutputRow OrchConductorAudioProcessor::getWoodwindsOutputRow (int index) const
{
    if (index < 0 || index >= numWoodwindsRows)
        return { "Invalid", 0, 0 };

    return
    {
        woodwindsInstrumentNames[index],
        woodwindsCcNumbers[index],
        getWoodwindsPresetValueForIndex (index)
    };
}

int OrchConductorAudioProcessor::getNumBrassOutputRows()
{
    return numBrassRows;
}

OrchConductorAudioProcessor::OutputRow OrchConductorAudioProcessor::getBrassOutputRow (int index) const
{
    if (index < 0 || index >= numBrassRows)
        return { "Invalid", 0, 0 };

    return
    {
        brassInstrumentNames[index],
        brassCcNumbers[index],
        getBrassPresetValueForIndex (index)
    };
}

int OrchConductorAudioProcessor::getNumPercussionOutputRows()
{
    return numPercussionRows;
}

OrchConductorAudioProcessor::OutputRow OrchConductorAudioProcessor::getPercussionOutputRow (int index) const
{
    if (index < 0 || index >= numPercussionRows)
        return { "Invalid", 0, 0 };

    return
    {
        percussionInstrumentNames[index],
        percussionCcNumbers[index],
        getPercussionPresetValueForIndex (index)
    };
}

int OrchConductorAudioProcessor::getPresetValueForIndex (int index) const
{
    if (index < 0 || index >= numRows)
        return 0;

    switch (getPreset())
    {
        case Preset::allOff:        return 0;
        case Preset::violinIOnly:   return index == 0 ? 127 : 0;
        case Preset::violinIIOnly:  return index == 1 ? 127 : 0;
        case Preset::violinsOnly:   return index <= 1 ? 127 : 0;
        case Preset::violasOnly:    return index == 2 ? 127 : 0;
        case Preset::cellosOnly:    return index == 3 ? 127 : 0;
        case Preset::bassesOnly:    return index == 4 ? 127 : 0;
        case Preset::upperStrings:  return index <= 2 ? 127 : 0;

        case Preset::lowStrings:
            if (index == 2) return 64;
            if (index == 3) return 127;
            if (index == 4) return 127;
            return 0;

        case Preset::stringQuartet: return index <= 3 ? 127 : 0;

        case Preset::violaCello:
            if (index == 2) return 127;
            if (index == 3) return 127;
            return 0;

        case Preset::celloBass:
            if (index == 3) return 127;
            if (index == 4) return 127;
            return 0;

        case Preset::fullStrings:   return 127;
        case Preset::tutti:         return 127;
    }

    return 0;
}

int OrchConductorAudioProcessor::getWoodwindsPresetValueForIndex (int index) const
{
    if (index < 0 || index >= numWoodwindsRows)
        return 0;

    switch (woodwindsPresetId)
    {
        case 0: return 0;                         // All Off
        case 1: return index == 0 ? 127 : 0;      // Piccolo Only

        case 2:                                  // Flutes
            if (index == 1) return 127;
            if (index == 2) return 127;
            return 0;

        case 3:                                  // Reeds
            if (index >= 3 && index <= 10) return 127;
            return 0;

        case 4:                                  // Low Woodwinds
            if (index == 8) return 127;
            if (index == 9) return 127;
            if (index == 10) return 127;
            if (index == 11) return 127;
            return 0;

        case 5: return 127;                      // Full Woodwinds
    }

    return 0;
}

int OrchConductorAudioProcessor::getBrassPresetValueForIndex (int index) const
{
    if (index < 0 || index >= numBrassRows)
        return 0;

    switch (brassPresetId)
    {
        case 0: return 0;                         // All Off

        case 1:                                  // Horns
            if (index >= 0 && index <= 3) return 127;
            return 0;

        case 2:                                  // Trumpets
            if (index >= 4 && index <= 6) return 127;
            return 0;

        case 3:                                  // Trombones
            if (index == 7) return 127;
            if (index == 8) return 127;
            return 0;

        case 4:                                  // Low Brass
            if (index == 7) return 127;
            if (index == 8) return 127;
            if (index == 9) return 127;
            if (index == 10) return 127;
            return 0;

        case 5: return 127;                      // Full Brass
    }

    return 0;
}

int OrchConductorAudioProcessor::getPercussionPresetValueForIndex (int index) const
{
    if (index < 0 || index >= numPercussionRows)
        return 0;

    switch (percussionPresetId)
    {
        case 0: return 0;
        case 1: return index == 0 ? 127 : 0; // Timpani
        case 2: return index == 1 ? 127 : 0; // Glockenspiel
        case 3: return index == 2 ? 127 : 0; // Xylophone

        case 4: // Mallets: Xylophone, Marimba, Vibraphone
            if (index == 2) return 127;
            if (index == 3) return 127;
            if (index == 4) return 127;
            return 0;

        case 5: return 127; // Full Melodic Percussion
    }

    return 0;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrchConductorAudioProcessor();
}



