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

    const bool useCombi = isCombiModeActive() && ! shouldSendAllOff;

    for (int i = 0; i < numWoodwindsRows; ++i)
    {
        const int cc = woodwindsCcNumbers[i];
        const int value = shouldSendAllOff ? 0
                         : useCombi       ? getCombiPresetValueForCc (cc)
                                          : getWoodwindsPresetValueForIndex (i);

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    for (int i = 0; i < numBrassRows; ++i)
    {
        const int cc = brassCcNumbers[i];
        const int value = shouldSendAllOff ? 0
                         : useCombi       ? getCombiPresetValueForCc (cc)
                                          : getBrassPresetValueForIndex (i);

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    for (int i = 0; i < numPercussionRows; ++i)
    {
        const int cc = percussionCcNumbers[i];
        const int value = shouldSendAllOff ? 0
                         : useCombi       ? getCombiPresetValueForCc (cc)
                                          : getPercussionPresetValueForIndex (i);

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, 49, 0), 0);

    for (int i = 0; i < numRows; ++i)
    {
        const int cc = ccNumbers[i];
        const int value = shouldSendAllOff ? 0
                         : useCombi       ? getCombiPresetValueForCc (cc)
                                          : getPresetValueForIndex (i);

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
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
    {
        const bool changed = combiPresetId != presetId;
        combiPresetId = presetId;

        if (changed && sendOnPresetChange)
            requestSendPreset();
    }
}


bool OrchConductorAudioProcessor::isCombiModeActive() const
{
    return combiPresetId != static_cast<int> (CombiPreset::manualSections);
}

juce::String OrchConductorAudioProcessor::getCombiPresetName() const
{
    switch (static_cast<CombiPreset> (combiPresetId))
    {
        case CombiPreset::manualSections: return "Manual Sections";

        case CombiPreset::utilityAllOff: return "[Utility] All Off";
        case CombiPreset::utilityFullOrchestra: return "[Utility] Full Orchestra";
        case CombiPreset::utilityFullOrchestraNoPercussion: return "[Utility] Full Orchestra No Percussion";
        case CombiPreset::utilityChamberOrchestra: return "[Utility] Chamber Orchestra";
        case CombiPreset::utilityFullStrings: return "[Utility] Full Strings";
        case CombiPreset::utilityFullWoodwinds: return "[Utility] Full Woodwinds";
        case CombiPreset::utilityFullBrass: return "[Utility] Full Brass";
        case CombiPreset::utilityFullWinds: return "[Utility] Full Winds";
        case CombiPreset::utilityHighOrchestra: return "[Utility] High Orchestra";
        case CombiPreset::utilityLowOrchestra: return "[Utility] Low Orchestra";
        case CombiPreset::utilityMiddleOrchestra: return "[Utility] Middle Orchestra";

        case CombiPreset::romanticWarmStringsHorns: return "[Romantic] Warm Strings + Horns";
        case CombiPreset::romanticOboeStrings: return "[Romantic] Oboe + Strings";
        case CombiPreset::romanticFluteViolins: return "[Romantic] Flute + Violins";
        case CombiPreset::romanticBassoonCelli: return "[Romantic] Bassoon + Celli";
        case CombiPreset::romanticHornChoirStrings: return "[Romantic] Horn Choir + Strings";

        case CombiPreset::cinematicHeroicBrassStrings: return "[Cinematic] Heroic Brass + Strings";
        case CombiPreset::cinematicDarkTrailerBed: return "[Cinematic] Dark Trailer Bed";
        case CombiPreset::cinematicHighWindsShimmer: return "[Cinematic] High Winds Shimmer";
        case CombiPreset::cinematicEpicLowPulse: return "[Cinematic] Epic Low Pulse";

        case CombiPreset::herrmannLowReeds: return "[Herrmann] Low Reeds";
        case CombiPreset::herrmannHornKnives: return "[Herrmann] Horn Knives";
        case CombiPreset::herrmannPsychoStrings: return "[Herrmann] Psycho Strings";
        case CombiPreset::herrmannSuspenseWinds: return "[Herrmann] Suspense Winds";

        case CombiPreset::modernistPointillistWinds: return "[Modernist] Pointillist Winds";
        case CombiPreset::modernistSparseExtremes: return "[Modernist] Sparse Extremes";
        case CombiPreset::shimmerSilverShimmer: return "[Shimmer] Silver Shimmer";
        case CombiPreset::soloEnglishHornLament: return "[Solo] English Horn Lament";
    }

    return "Unknown Combi";
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
    bool changed = false;

    switch (section)
    {
        case Section::woodwinds:
            if (presetId >= minSectionPresetId && presetId <= maxWoodwindsPresetId)
            {
                woodwindsPresetId = presetId;
                changed = true;
            }
            break;

        case Section::brass:
            if (presetId >= minSectionPresetId && presetId <= maxBrassPresetId)
            {
                brassPresetId = presetId;
                changed = true;
            }
            break;

        case Section::percussion:
            if (presetId >= minSectionPresetId && presetId <= maxPercussionPresetId)
            {
                percussionPresetId = presetId;
                changed = true;
            }
            break;

        case Section::strings:
            if (presetId >= minStringsPresetId && presetId <= maxStringsPresetId)
            {
                stringsPresetId = presetId;
                changed = true;
            }
            break;
    }

    if (changed && sendOnPresetChange)
        requestSendPreset();
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
            return (index == 1 || index == 2) ? 127 : 0;
        case 3: return index == 1 ? 127 : 0;      // Flute 1 Only
        case 4: return index == 2 ? 127 : 0;      // Flute 2 Only

        case 5:                                  // Oboes
            return (index == 3 || index == 4) ? 127 : 0;
        case 6: return index == 3 ? 127 : 0;      // Oboe 1 Only
        case 7: return index == 4 ? 127 : 0;      // Oboe 2 Only
        case 8: return index == 5 ? 127 : 0;      // English Horn Only

        case 9:                                  // Clarinets
            return (index == 6 || index == 7) ? 127 : 0;
        case 10: return index == 6 ? 127 : 0;     // Clarinet 1 Only
        case 11: return index == 7 ? 127 : 0;     // Clarinet 2 Only
        case 12: return index == 8 ? 127 : 0;     // Bass Clarinet Only

        case 13:                                 // Bassoons
            return (index == 9 || index == 10) ? 127 : 0;
        case 14: return index == 9 ? 127 : 0;     // Bassoon 1 Only
        case 15: return index == 10 ? 127 : 0;    // Bassoon 2 Only
        case 16: return index == 11 ? 127 : 0;    // Contrabassoon Only

        case 17:                                 // High Woodwinds
            return (index >= 0 && index <= 7) ? 127 : 0;

        case 18:                                 // Low Woodwinds
            return (index >= 8 && index <= 11) ? 127 : 0;

        case 19: return 127;                     // Full Woodwinds
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
            return (index >= 0 && index <= 3) ? 127 : 0;
        case 2: return index == 0 ? 127 : 0;      // Horn 1 Only
        case 3: return index == 1 ? 127 : 0;      // Horn 2 Only
        case 4: return index == 2 ? 127 : 0;      // Horn 3 Only
        case 5: return index == 3 ? 127 : 0;      // Horn 4 Only

        case 6:                                  // Trumpets
            return (index >= 4 && index <= 6) ? 127 : 0;
        case 7: return index == 4 ? 127 : 0;      // Trumpet 1 Only
        case 8: return index == 5 ? 127 : 0;      // Trumpet 2 Only
        case 9: return index == 6 ? 127 : 0;      // Trumpet 3 Only

        case 10:                                 // Trombones
            return (index == 7 || index == 8) ? 127 : 0;
        case 11: return index == 7 ? 127 : 0;     // Trombone 1 Only
        case 12: return index == 8 ? 127 : 0;     // Trombone 2 Only
        case 13: return index == 9 ? 127 : 0;     // Bass Trombone Only

        case 14: return index == 10 ? 127 : 0;    // Tuba Only

        case 15:                                 // Low Brass
            return (index >= 7 && index <= 10) ? 127 : 0;

        case 16: return 127;                     // Full Brass
    }

    return 0;
}

int OrchConductorAudioProcessor::getPercussionPresetValueForIndex (int index) const
{
    if (index < 0 || index >= numPercussionRows)
        return 0;

    switch (percussionPresetId)
    {
        case 0: return 0;                         // All Off
        case 1: return index == 0 ? 127 : 0;      // Timpani Only
        case 2: return index == 1 ? 127 : 0;      // Glockenspiel Only
        case 3: return index == 2 ? 127 : 0;      // Xylophone Only
        case 4: return index == 3 ? 127 : 0;      // Marimba Only
        case 5: return index == 4 ? 127 : 0;      // Vibraphone Only
        case 6: return index == 5 ? 127 : 0;      // Tubular Bells Only

        case 7:                                  // Mallets
            return (index >= 1 && index <= 5) ? 127 : 0;

        case 8: return 127;                      // Full Melodic Percussion
    }

    return 0;
}


int OrchConductorAudioProcessor::getCombiPresetValueForCc (int ccNumber) const
{
    if (ccNumber == 49)
        return 0; // Harp reserved.

    const auto combi = static_cast<CombiPreset> (combiPresetId);

    switch (combi)
    {
        case CombiPreset::manualSections:
            return 0;

        case CombiPreset::utilityAllOff:
            return 0;

        case CombiPreset::utilityFullOrchestra:
            return ((ccNumber >= 20 && ccNumber <= 48) || (ccNumber >= 50 && ccNumber <= 54)) ? 127 : 0;

        case CombiPreset::utilityFullOrchestraNoPercussion:
            return ((ccNumber >= 20 && ccNumber <= 42) || (ccNumber >= 50 && ccNumber <= 54)) ? 127 : 0;

        case CombiPreset::utilityChamberOrchestra:
            return (ccNumber == 21 || ccNumber == 23 || ccNumber == 26 || ccNumber == 29 || ccNumber == 32
                 || ccNumber == 50 || ccNumber == 51 || ccNumber == 52 || ccNumber == 53 || ccNumber == 54) ? 127 : 0;

        case CombiPreset::utilityFullStrings:
            return (ccNumber >= 50 && ccNumber <= 54) ? 127 : 0;

        case CombiPreset::utilityFullWoodwinds:
            return (ccNumber >= 20 && ccNumber <= 31) ? 127 : 0;

        case CombiPreset::utilityFullBrass:
            return (ccNumber >= 32 && ccNumber <= 42) ? 127 : 0;

        case CombiPreset::utilityFullWinds:
            return (ccNumber >= 20 && ccNumber <= 42) ? 127 : 0;

        case CombiPreset::utilityHighOrchestra:
            return (ccNumber == 20 || ccNumber == 21 || ccNumber == 22 || ccNumber == 23 || ccNumber == 24
                 || ccNumber == 36 || ccNumber == 37 || ccNumber == 38
                 || ccNumber == 44 || ccNumber == 45 || ccNumber == 47 || ccNumber == 48
                 || ccNumber == 50 || ccNumber == 51) ? 127 : 0;

        case CombiPreset::utilityLowOrchestra:
            return (ccNumber == 28 || ccNumber == 29 || ccNumber == 30 || ccNumber == 31
                 || ccNumber == 39 || ccNumber == 40 || ccNumber == 41 || ccNumber == 42
                 || ccNumber == 43 || ccNumber == 52 || ccNumber == 53 || ccNumber == 54) ? 127 : 0;

        case CombiPreset::utilityMiddleOrchestra:
            return (ccNumber == 25 || ccNumber == 26 || ccNumber == 27
                 || ccNumber == 32 || ccNumber == 33 || ccNumber == 34 || ccNumber == 35
                 || ccNumber == 46 || ccNumber == 47
                 || ccNumber == 51 || ccNumber == 52 || ccNumber == 53) ? 127 : 0;

        case CombiPreset::romanticWarmStringsHorns:
            return ((ccNumber >= 32 && ccNumber <= 35) || (ccNumber >= 50 && ccNumber <= 54)) ? 127 : 0;

        case CombiPreset::romanticOboeStrings:
            return (ccNumber == 23 || ccNumber == 50 || ccNumber == 51 || ccNumber == 52 || ccNumber == 53) ? 127 : 0;

        case CombiPreset::romanticFluteViolins:
            return (ccNumber == 21 || ccNumber == 22 || ccNumber == 50 || ccNumber == 51) ? 127 : 0;

        case CombiPreset::romanticBassoonCelli:
            return (ccNumber == 29 || ccNumber == 30 || ccNumber == 53 || ccNumber == 54) ? 127 : 0;

        case CombiPreset::romanticHornChoirStrings:
            return ((ccNumber >= 32 && ccNumber <= 35) || (ccNumber >= 50 && ccNumber <= 54)) ? 127 : 0;

        case CombiPreset::cinematicHeroicBrassStrings:
            return ((ccNumber >= 32 && ccNumber <= 42) || (ccNumber >= 50 && ccNumber <= 54) || ccNumber == 43) ? 127 : 0;

        case CombiPreset::cinematicDarkTrailerBed:
            return (ccNumber == 28 || ccNumber == 31 || ccNumber == 41 || ccNumber == 42
                 || ccNumber == 43 || ccNumber == 53 || ccNumber == 54) ? 127 : 0;

        case CombiPreset::cinematicHighWindsShimmer:
            return (ccNumber == 20 || ccNumber == 21 || ccNumber == 22
                 || ccNumber == 44 || ccNumber == 47 || ccNumber == 48
                 || ccNumber == 50 || ccNumber == 51) ? 127 : 0;

        case CombiPreset::cinematicEpicLowPulse:
            return (ccNumber == 28 || ccNumber == 31 || ccNumber == 39 || ccNumber == 40 || ccNumber == 41 || ccNumber == 42
                 || ccNumber == 43 || ccNumber == 53 || ccNumber == 54) ? 127 : 0;

        case CombiPreset::herrmannLowReeds:
            return (ccNumber == 28 || ccNumber == 29 || ccNumber == 30 || ccNumber == 31 || ccNumber == 53 || ccNumber == 54) ? 127 : 0;

        case CombiPreset::herrmannHornKnives:
            return (ccNumber == 32 || ccNumber == 33 || ccNumber == 34 || ccNumber == 35 || ccNumber == 52 || ccNumber == 53) ? 127 : 0;

        case CombiPreset::herrmannPsychoStrings:
            return (ccNumber == 50 || ccNumber == 51 || ccNumber == 52 || ccNumber == 53) ? 127 : 0;

        case CombiPreset::herrmannSuspenseWinds:
            return (ccNumber == 23 || ccNumber == 25 || ccNumber == 26 || ccNumber == 28 || ccNumber == 29 || ccNumber == 30) ? 127 : 0;

        case CombiPreset::modernistPointillistWinds:
            return (ccNumber == 20 || ccNumber == 23 || ccNumber == 26 || ccNumber == 29) ? 127 : 0;

        case CombiPreset::modernistSparseExtremes:
            return (ccNumber == 20 || ccNumber == 31 || ccNumber == 36 || ccNumber == 41 || ccNumber == 45 || ccNumber == 50 || ccNumber == 54) ? 127 : 0;

        case CombiPreset::shimmerSilverShimmer:
            return (ccNumber == 20 || ccNumber == 21 || ccNumber == 22
                 || ccNumber == 44 || ccNumber == 47 || ccNumber == 48
                 || ccNumber == 50 || ccNumber == 51) ? 127 : 0;

        case CombiPreset::soloEnglishHornLament:
            if (ccNumber == 25) return 127;
            if (ccNumber == 52) return 127;
            if (ccNumber == 53) return 127;
            if (ccNumber == 54) return 64;
            return 0;
    }

    return 0;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrchConductorAudioProcessor();
}



