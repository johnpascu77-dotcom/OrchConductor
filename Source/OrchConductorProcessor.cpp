#include "OrchConductorProcessor.h"
#include <algorithm>
#include <cmath>
#include "OrchConductorEditor.h"
#include "OrchConductorRuntimePresetSource.h"
#include "OrchConductorRuntimePresetCatalog.h"
#include "OrchConductorNarrativeScanResolver.h"
#include "OrchConductorEmbeddedFactoryJson.h"

namespace
{
    constexpr int runtimeSectionPresetAuditMinId = 0;
    constexpr int runtimeWoodwindPresetAuditMaxId = 19;
    constexpr int runtimeBrassPresetAuditMaxId = 16;
    constexpr int runtimePercussionPresetAuditMaxId = 8;
    constexpr int runtimeStringPresetAuditMaxId = 13;
    constexpr int runtimeCombiPresetAuditMinId = 0;
    constexpr int runtimeCombiPresetAuditMaxId = 28;
    // A combi with this many explicit CC values is treated as writing a
    // near-complete payload and is then required to pin the reserved harp
    // CC (49) to 0. Bumped 35 -> 43 on 2026-09-10: the addressable CC span
    // grew from 20-54 to 20-62 (the 7 unpitched percussion instruments), so
    // "Full Orchestra" legitimately carries 41 values now without ever
    // touching CC49.
    constexpr int runtimeCombiFullPayloadValueCount = 43;
    constexpr int runtimeNarrativeLaneExpectedCount = 6;
    constexpr int reservedHarpCcNumber = 49;
    constexpr int reservedHarpCcValue = 0;
    constexpr int pianoCcNumber = 55;

    // Phase 10F.5 MC bridge: undefined MIDI CCs (102-119 range), clear of both
    // OrchConductor's own CC20-54 output and MPL's CC20-64 external-control map.
    // Read on any channel from input MIDI; passed through untouched.
    constexpr int narrativePositionCcNumber = 102;
    constexpr int narrativeLaneCcNumber = 103;
    constexpr int authorityModeCcNumber = 104;

    // OrchGate response bridge (broadcast, read by every OrchGate set to
    // "Follow Conductor Response"). Also undefined controllers, clear of the
    // CC20-62 send map and CC102-105.
    constexpr int gateResponseModeCcDefault = 106;
    constexpr int gateResponseAmountCcDefault = 107;

    struct ExpectedRuntimeCatalogValue
    {
        int ccNumber = -1;
        int value = -1;
    };

    // Phase 6D: per-CC lookup helpers.
    // The factory JSON uses sparse encoding: zero-value CCs are omitted.
    // These helpers scan stored values by CC number and return 0 for any CC absent from the catalog.
    // This matches the runtime payload semantic: omitted CC => value 0.

    int catalogSectionPresetValueForCc(const OrchConductorRuntimePresetCatalog& catalog,
                                       const juce::String& sectionId,
                                       int presetIndex,
                                       int ccNumber)
    {
        const int count = catalog.getSectionPresetValueCount(sectionId, presetIndex);

        for (int i = 0; i < count; ++i)
        {
            const auto v = catalog.getSectionPresetValue(sectionId, presetIndex, i);

            if (v.isValid && v.ccNumber == ccNumber)
                return v.value;
        }

        return 0;
    }

    int catalogCombiPresetValueForCc(const OrchConductorRuntimePresetCatalog& catalog,
                                     int presetIndex,
                                     int ccNumber)
    {
        const int count = catalog.getCombiPresetValueCount(presetIndex);

        for (int i = 0; i < count; ++i)
        {
            const auto v = catalog.getCombiPresetValue(presetIndex, i);

            if (v.isValid && v.ccNumber == ccNumber)
                return v.value;
        }

        return 0;
    }

    bool sectionPresetCcMatches(const OrchConductorRuntimePresetCatalog& catalog,
                                const juce::String& sectionId,
                                int presetIndex,
                                const ExpectedRuntimeCatalogValue* expectedValues,
                                int expectedValueCount)
    {
        for (int i = 0; i < expectedValueCount; ++i)
        {
            if (catalogSectionPresetValueForCc(catalog, sectionId, presetIndex, expectedValues[i].ccNumber)
                    != expectedValues[i].value)
                return false;
        }

        return true;
    }

    bool combiPresetCcMatches(const OrchConductorRuntimePresetCatalog& catalog,
                              int presetIndex,
                              const ExpectedRuntimeCatalogValue* expectedValues,
                              int expectedValueCount)
    {
        for (int i = 0; i < expectedValueCount; ++i)
        {
            if (catalogCombiPresetValueForCc(catalog, presetIndex, expectedValues[i].ccNumber)
                    != expectedValues[i].value)
                return false;
        }

        return true;
    }
    bool runtimeCatalogValueIsMidiSafe(const OrchConductorRuntimePresetValueView& value)
    {
        return value.isValid
            && value.ccNumber >= 0
            && value.ccNumber <= 127
            && value.value >= 0
            && value.value <= 127;
    }

    bool tryAssignRuntimeCatalogValueForCc(const OrchConductorRuntimePresetValueView& runtimeValue,
                                            int ccNumber,
                                            int& value)
    {
        if (runtimeCatalogValueIsMidiSafe (runtimeValue)
            && runtimeValue.ccNumber == ccNumber)
        {
            value = runtimeValue.value;
            return true;
        }

        return false;
    }

    bool verifyRuntimeCatalogSectionPresetRange(const OrchConductorRuntimePresetCatalog& catalog,
                                                const juce::String& sectionId,
                                                int firstPresetId,
                                                int lastPresetId)
    {
        for (int presetId = firstPresetId; presetId <= lastPresetId; ++presetId)
        {
            const int valueCount = catalog.getSectionPresetValueCount(sectionId, presetId);

            if (valueCount < 0)
                return false;

            for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex)
            {
                if (! runtimeCatalogValueIsMidiSafe(catalog.getSectionPresetValue(sectionId, presetId, valueIndex)))
                    return false;
            }
        }

        return true;
    }

    bool verifyRuntimeCatalogCombiPresetRange(const OrchConductorRuntimePresetCatalog& catalog,
                                              int firstPresetId,
                                              int lastPresetId)
    {
        for (int presetId = firstPresetId; presetId <= lastPresetId; ++presetId)
        {
            const int valueCount = catalog.getCombiPresetValueCount(presetId);

            if (valueCount < 0)
                return false;

            bool sawReservedHarpCc = false;

            for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex)
            {
                const auto value = catalog.getCombiPresetValue(presetId, valueIndex);

                if (! runtimeCatalogValueIsMidiSafe(value))
                    return false;

                if (value.ccNumber == reservedHarpCcNumber)
                {
                    sawReservedHarpCc = true;

                    if (value.value != reservedHarpCcValue)
                        return false;
                }
            }

            if (valueCount >= runtimeCombiFullPayloadValueCount && ! sawReservedHarpCc)
                return false;
        }

        return true;
    }

    int findRuntimeCatalogNarrativeLaneIndex(const OrchConductorRuntimePresetCatalog& catalog,
                                             const juce::String& laneId)
    {
        const int laneCount = catalog.getNarrativeLaneCount();

        for (int laneIndex = 0; laneIndex < laneCount; ++laneIndex)
        {
            if (catalog.getNarrativeLaneId(laneIndex) == laneId)
                return laneIndex;
        }

        return -1;
    }

    bool verifyRuntimeCatalogNarrativeLanePoints(const OrchConductorRuntimePresetCatalog& catalog,
                                                 int laneIndex)
    {
        const int pointCount = catalog.getNarrativeLanePointCount(laneIndex);

        if (pointCount <= 0)
            return false;

        double previousPosition = -1.0;

        for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
        {
            const auto position = catalog.getNarrativeLanePointPosition(laneIndex, pointIndex);
            const auto combiId = catalog.getNarrativeLanePointCombiId(laneIndex, pointIndex);

            if (position < 0.0 || position > 1.0)
                return false;

            if (position <= previousPosition)
                return false;

            if (combiId < runtimeCombiPresetAuditMinId || combiId > runtimeCombiPresetAuditMaxId)
                return false;

            previousPosition = position;
        }

        return true;
    }

    bool verifyRuntimeCatalogNarrativeLanes(const OrchConductorRuntimePresetCatalog& catalog)
    {
        if (catalog.getNarrativeLaneCount() != runtimeNarrativeLaneExpectedCount)
            return false;

        for (int laneIndex = 0; laneIndex < catalog.getNarrativeLaneCount(); ++laneIndex)
        {
            if (catalog.getNarrativeLaneId(laneIndex).isEmpty())
                return false;

            if (catalog.getNarrativeLaneLabel(laneIndex).isEmpty())
                return false;

            if (! verifyRuntimeCatalogNarrativeLanePoints(catalog, laneIndex))
                return false;
        }

        const int organicBuildLaneIndex =
            findRuntimeCatalogNarrativeLaneIndex(catalog, "organic_build");

        if (organicBuildLaneIndex < 0)
            return false;

        if (catalog.getNarrativeLanePointCount(organicBuildLaneIndex) != 6)
            return false;

        const int anticlimaxLaneIndex =
            findRuntimeCatalogNarrativeLaneIndex(catalog, "anticlimax");

        if (anticlimaxLaneIndex < 0)
            return false;

        const int anticlimaxPointCount =
            catalog.getNarrativeLanePointCount(anticlimaxLaneIndex);

        if (anticlimaxPointCount <= 0)
            return false;

        return catalog.getNarrativeLanePointCombiId(anticlimaxLaneIndex,
                                                    anticlimaxPointCount - 1) == 1;
    }

    bool verifyRuntimeCatalogCoverageAudit(const OrchConductorRuntimePresetCatalog& catalog)
    {
        return verifyRuntimeCatalogSectionPresetRange(catalog, "woodwinds",
                                                  runtimeSectionPresetAuditMinId,
                                                  runtimeWoodwindPresetAuditMaxId)
            && verifyRuntimeCatalogSectionPresetRange(catalog, "brass",
                                                  runtimeSectionPresetAuditMinId,
                                                  runtimeBrassPresetAuditMaxId)
            && verifyRuntimeCatalogSectionPresetRange(catalog, "percussion",
                                                  runtimeSectionPresetAuditMinId,
                                                  runtimePercussionPresetAuditMaxId)
            && verifyRuntimeCatalogSectionPresetRange(catalog, "strings",
                                                  runtimeSectionPresetAuditMinId,
                                                  runtimeStringPresetAuditMaxId)
            && verifyRuntimeCatalogCombiPresetRange(catalog,
                                                    runtimeCombiPresetAuditMinId,
                                                    runtimeCombiPresetAuditMaxId);
    }

    bool verifyRuntimeCatalogAuthorityTrialSentinels(const OrchConductorRuntimePresetCatalog& catalog)
    {
        // Phase 5G authority-trial sentinels.
        // This is still diagnostic-only. It does not route MIDI or change preset authority.
        //
        // Phase 6D: rewritten to compare by CC number using per-CC lookup.
        // Omitted CCs resolve to 0, matching runtime dense-payload semantics.

        // strings preset 8 "Low Strings": CC50=0, CC51=0, CC52=64, CC53=127, CC54=127
        const ExpectedRuntimeCatalogValue lowStringsExpected[] =
        {
            { 50, 0 }, { 51, 0 }, { 52, 64 }, { 53, 127 }, { 54, 127 }
        };

        // strings preset 12 "Full Strings": CC50-54 all 127
        const ExpectedRuntimeCatalogValue fullStringsExpected[] =
        {
            { 50, 127 }, { 51, 127 }, { 52, 127 }, { 53, 127 }, { 54, 127 }
        };

        // combi preset 28 "[Solo] English Horn Lament": only CC25, CC52, CC53, CC54 non-zero
        const ExpectedRuntimeCatalogValue soloEnglishHornLamentExpected[] =
        {
            { 20, 0 },   { 21, 0 },   { 22, 0 },   { 23, 0 },
            { 24, 0 },   { 25, 127 }, { 26, 0 },   { 27, 0 },
            { 28, 0 },   { 29, 0 },   { 30, 0 },   { 31, 0 },
            { 32, 0 },   { 33, 0 },   { 34, 0 },   { 35, 0 },
            { 36, 0 },   { 37, 0 },   { 38, 0 },   { 39, 0 },
            { 40, 0 },   { 41, 0 },   { 42, 0 },   { 43, 0 },
            { 44, 0 },   { 45, 0 },   { 46, 0 },   { 47, 0 },
            { 48, 0 },   { 49, 0 },   { 50, 0 },   { 51, 0 },
            { 52, 127 }, { 53, 127 }, { 54, 64 }
        };

        return sectionPresetCcMatches(catalog, "strings", 8, lowStringsExpected,
                                      static_cast<int>(std::size(lowStringsExpected)))
            && sectionPresetCcMatches(catalog, "strings", 12, fullStringsExpected,
                                      static_cast<int>(std::size(fullStringsExpected)))
            && combiPresetCcMatches(catalog, 28, soloEnglishHornLamentExpected,
                                    static_cast<int>(std::size(soloEnglishHornLamentExpected)));
    }


    bool verifyRuntimeCatalogPayloadEquivalenceSentinels(const OrchConductorRuntimePresetCatalog& catalog)
    {
        // Phase 6D: rewritten to compare by CC number using per-CC lookup.
        // Omitted CCs resolve to 0, matching runtime dense-payload semantics.
        // The factory JSON is sparse; zero-value CCs are not stored.

        // strings preset 8 "Low Strings": CC50=0, CC51=0, CC52=64, CC53=127, CC54=127
        const ExpectedRuntimeCatalogValue lowStringsExpected[] =
        {
            { 50, 0 }, { 51, 0 }, { 52, 64 }, { 53, 127 }, { 54, 127 }
        };

        // woodwinds preset 19 "Full Woodwinds": CC20-31 all 127
        const ExpectedRuntimeCatalogValue fullWoodwindsExpected[] =
        {
            { 20, 127 }, { 21, 127 }, { 22, 127 }, { 23, 127 },
            { 24, 127 }, { 25, 127 }, { 26, 127 }, { 27, 127 },
            { 28, 127 }, { 29, 127 }, { 30, 127 }, { 31, 127 }
        };

        // brass preset 16 "Full Brass": CC32-42 all 127
        const ExpectedRuntimeCatalogValue fullBrassExpected[] =
        {
            { 32, 127 }, { 33, 127 }, { 34, 127 }, { 35, 127 },
            { 36, 127 }, { 37, 127 }, { 38, 127 }, { 39, 127 },
            { 40, 127 }, { 41, 127 }, { 42, 127 }
        };

        // percussion preset 8 "Full Melodic Percussion": CC43-48 all 127
        const ExpectedRuntimeCatalogValue fullMelodicPercussionExpected[] =
        {
            { 43, 127 }, { 44, 127 }, { 45, 127 },
            { 46, 127 }, { 47, 127 }, { 48, 127 }
        };

        // strings preset 12 "Full Strings": CC50-54 all 127
        const ExpectedRuntimeCatalogValue fullStringsExpected[] =
        {
            { 50, 127 }, { 51, 127 }, { 52, 127 }, { 53, 127 }, { 54, 127 }
        };

        // combi preset 2 "[Utility] Full Orchestra": CC20-48 all 127, CC49=0 (reserved), CC50-54 all 127
        const ExpectedRuntimeCatalogValue fullOrchestraExpected[] =
        {
            { 20, 127 }, { 21, 127 }, { 22, 127 }, { 23, 127 }, { 24, 127 },
            { 25, 127 }, { 26, 127 }, { 27, 127 }, { 28, 127 }, { 29, 127 },
            { 30, 127 }, { 31, 127 }, { 32, 127 }, { 33, 127 }, { 34, 127 },
            { 35, 127 }, { 36, 127 }, { 37, 127 }, { 38, 127 }, { 39, 127 },
            { 40, 127 }, { 41, 127 }, { 42, 127 }, { 43, 127 }, { 44, 127 },
            { 45, 127 }, { 46, 127 }, { 47, 127 }, { 48, 127 }, { 49, 0 },
            { 50, 127 }, { 51, 127 }, { 52, 127 }, { 53, 127 }, { 54, 127 }
        };

        // combi preset 28 "[Solo] English Horn Lament": only CC25, CC52, CC53, CC54 non-zero
        const ExpectedRuntimeCatalogValue soloEnglishHornLamentExpected[] =
        {
            { 20, 0 },   { 21, 0 },   { 22, 0 },   { 23, 0 },
            { 24, 0 },   { 25, 127 }, { 26, 0 },   { 27, 0 },
            { 28, 0 },   { 29, 0 },   { 30, 0 },   { 31, 0 },
            { 32, 0 },   { 33, 0 },   { 34, 0 },   { 35, 0 },
            { 36, 0 },   { 37, 0 },   { 38, 0 },   { 39, 0 },
            { 40, 0 },   { 41, 0 },   { 42, 0 },   { 43, 0 },
            { 44, 0 },   { 45, 0 },   { 46, 0 },   { 47, 0 },
            { 48, 0 },   { 49, 0 },   { 50, 0 },   { 51, 0 },
            { 52, 127 }, { 53, 127 }, { 54, 64 }
        };

        return sectionPresetCcMatches(catalog, "strings", 8, lowStringsExpected,
                                      static_cast<int>(std::size(lowStringsExpected)))
            && sectionPresetCcMatches(catalog, "woodwinds", 19, fullWoodwindsExpected,
                                      static_cast<int>(std::size(fullWoodwindsExpected)))
            && sectionPresetCcMatches(catalog, "brass", 16, fullBrassExpected,
                                      static_cast<int>(std::size(fullBrassExpected)))
            && sectionPresetCcMatches(catalog, "percussion", 8, fullMelodicPercussionExpected,
                                      static_cast<int>(std::size(fullMelodicPercussionExpected)))
            && sectionPresetCcMatches(catalog, "strings", 12, fullStringsExpected,
                                      static_cast<int>(std::size(fullStringsExpected)))
            && combiPresetCcMatches(catalog, 2, fullOrchestraExpected,
                                    static_cast<int>(std::size(fullOrchestraExpected)))
            && combiPresetCcMatches(catalog, 28, soloEnglishHornLamentExpected,
                                    static_cast<int>(std::size(soloEnglishHornLamentExpected)));
    }
    
    OrchConductorAudioProcessor::OutputRow makeOutputRow (const juce::String& instrumentName, int ccNumber, int value)
    {
        auto getMaxPlayers = [] (int cc) -> int
        {
            // Phase 2B default ensemble profile:
            // Winds, brass, percussion, and harp are single chairs.
            // Strings use compact default section sizes.
            switch (cc)
            {
                case 50: return 8; // Violin I
                case 51: return 6; // Violin II
                case 52: return 4; // Viola
                case 53: return 4; // Cello
                case 54: return 2; // Double Bass
                default: return 1;
            }
        };

        const int maxPlayers = getMaxPlayers (ccNumber);

        int activePlayers = 0;
        if (value > 0 && maxPlayers > 0)
        {
            const auto scaled = static_cast<int> (std::round ((static_cast<double> (value) / 127.0) * static_cast<double> (maxPlayers)));
            activePlayers = juce::jlimit (1, maxPlayers, scaled);
        }

        return { instrumentName, ccNumber, value, activePlayers, maxPlayers };
    }
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

    constexpr int numPercussionRows = 13;

    const char* percussionInstrumentNames[numPercussionRows] =
    {
        "Timpani",
        "Glockenspiel",
        "Xylophone",
        "Marimba",
        "Vibraphone",
        "Tubular Bells",
        // Unpitched percussion (CCs proposed in OrchPercMapper's CcMap -
        // see that repo's Design doc §7 for the fixed note-identity mapping
        // these instruments get downstream).
        "Bass Drum",
        "Snare Drum",
        "Cymbals",
        "Piatti",
        "Tam-Tam",
        "Tambourine",
        "Triangle"
    };

    constexpr int percussionCcNumbers[numPercussionRows] =
    {
        43, 44, 45, 46, 47, 48,
        56, 57, 58, 59, 60, 61, 62
    };
}

double clampNarrativeMetadata01(double value) noexcept
{
    if (value < 0.0)
        return 0.0;

    if (value > 1.0)
        return 1.0;

    return value;
}

double getObjectDoubleProperty(const juce::DynamicObject& object,
                               const juce::Identifier& propertyName,
                               double defaultValue)
{
    if (! object.hasProperty(propertyName))
        return defaultValue;

    return static_cast<double>(object.getProperty(propertyName));
}

juce::String getObjectStringProperty(const juce::DynamicObject& object,
                                     const juce::Identifier& propertyName,
                                     const juce::String& defaultValue)
{
    if (! object.hasProperty(propertyName))
        return defaultValue;

    return object.getProperty(propertyName).toString();
}

void readUserCombiNarrativeMetadata(const juce::DynamicObject& presetObject,
                                    orchconductor::NarrativeMetadata& destination)
{
    const auto metadata = presetObject.getProperty("metadata");

    if (! metadata.isObject())
        return;

    const auto* metadataObject = metadata.getDynamicObject();

    if (metadataObject == nullptr)
        return;

    destination.energy = clampNarrativeMetadata01(
        getObjectDoubleProperty(*metadataObject, "energy", destination.energy));

    destination.density = clampNarrativeMetadata01(
        getObjectDoubleProperty(*metadataObject, "density", destination.density));

    destination.brightness = clampNarrativeMetadata01(
        getObjectDoubleProperty(*metadataObject, "brightness", destination.brightness));

    destination.weight = clampNarrativeMetadata01(
        getObjectDoubleProperty(*metadataObject, "weight", destination.weight));

    destination.tension = clampNarrativeMetadata01(
        getObjectDoubleProperty(*metadataObject, "tension", destination.tension));

    const auto registerName = getObjectStringProperty(*metadataObject, "register", destination.registerName).trim();
    const auto role = getObjectStringProperty(*metadataObject, "role", destination.role).trim();
    const auto transitionBehavior = getObjectStringProperty(*metadataObject,
                                                            "transition_behavior",
                                                            destination.transitionBehavior).trim();
    const auto narrativeLane = getObjectStringProperty(*metadataObject,
                                                       "narrative_lane",
                                                       destination.narrativeLane).trim();

    if (registerName.isNotEmpty())
        destination.registerName = registerName;

    if (role.isNotEmpty())
        destination.role = role;

    if (transitionBehavior.isNotEmpty())
        destination.transitionBehavior = transitionBehavior;

    if (narrativeLane.isNotEmpty())
        destination.narrativeLane = narrativeLane;
}

juce::DynamicObject::Ptr createNarrativeMetadataJsonObject(
    const orchconductor::NarrativeMetadata& metadata)
{
    juce::DynamicObject::Ptr object = new juce::DynamicObject();

    object->setProperty("energy", metadata.energy);
    object->setProperty("density", metadata.density);
    object->setProperty("brightness", metadata.brightness);
    object->setProperty("weight", metadata.weight);
    object->setProperty("tension", metadata.tension);
    object->setProperty("register", metadata.registerName);
    object->setProperty("role", metadata.role);
    object->setProperty("transition_behavior", metadata.transitionBehavior);
    object->setProperty("narrative_lane", metadata.narrativeLane);

    return object;
}

OrchConductorAudioProcessor::OrchConductorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties())
#endif
{
    addParameter (combiPresetParameter = new juce::AudioParameterInt (
        juce::ParameterID { "combiPreset", 1 },
        "Combi Preset",
        minCombiPresetId,
        maxCombiPresetParameterId,
        static_cast<int> (CombiPreset::manualSections)));

    addParameter (woodwindsPresetParameter = new juce::AudioParameterInt (
        juce::ParameterID { "woodwindsPreset", 1 },
        "Woodwinds Preset",
        minSectionPresetId,
        maxWoodwindsPresetId,
        0));

    addParameter (brassPresetParameter = new juce::AudioParameterInt (
        juce::ParameterID { "brassPreset", 1 },
        "Brass Preset",
        minSectionPresetId,
        maxBrassPresetId,
        0));

    addParameter (percussionPresetParameter = new juce::AudioParameterInt (
        juce::ParameterID { "percussionPreset", 1 },
        "Percussion Preset",
        minSectionPresetId,
        maxPercussionPresetId,
        0));

    addParameter (stringsPresetParameter = new juce::AudioParameterInt (
        juce::ParameterID { "stringsPreset", 1 },
        "Strings Preset",
        minStringsPresetId,
        maxStringsPresetId,
        static_cast<int> (Preset::allOff)));

    addParameter (sendOnPresetChangeParameter = new juce::AudioParameterBool (
        juce::ParameterID { "sendOnPresetChange", 1 },
        "Send on Preset Change",
        sendOnPresetChange));

    addParameter (inputPassthroughParameter = new juce::AudioParameterChoice (
        juce::ParameterID { "inputPassthrough", 1 },
        "Input Passthrough",
        juce::StringArray { "Off", "Control CCs (>= 105)", "All" },
        static_cast<int> (inputPassthroughMode)));

    // Phase 10F.5 narrative-scan parameters. Appended last to keep existing
    // host automation slots stable. Not consumed by the send path in increment 1.
    addParameter (authorityModeParameter = new juce::AudioParameterChoice (
        juce::ParameterID { "authorityMode", 1 },
        "Authority Mode",
        juce::StringArray { "Manual Sections", "Combi Preset", "Narrative Scan" },
        static_cast<int> (AuthorityMode::manualSections)));

    addParameter (narrativeLaneParameter = new juce::AudioParameterInt (
        juce::ParameterID { "narrativeLane", 1 },
        "Narrative Lane",
        minNarrativeLaneParameterId,
        maxNarrativeLaneParameterId,
        0));

    addParameter (narrativePositionParameter = new juce::AudioParameterFloat (
        juce::ParameterID { "narrativePosition", 1 },
        "Narrative Position",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f));

    addParameter (fieldSelectCcParameter = new juce::AudioParameterInt (
        juce::ParameterID { "fieldSelectCc", 1 },
        "Field Select CC (0 = off)",
        0, 127, 105));

    // OrchGate response bridge broadcast CCs. Undefined controllers (106/107),
    // clear of OrchConductor's own CC20-62 + CC102-105 and of MPL's CC20-64.
    // 0 = don't broadcast that half of the bridge.
    addParameter (gateResponseModeCcParameter = new juce::AudioParameterInt (
        juce::ParameterID { "gateResponseModeCc", 1 },
        "Gate Response Mode CC (0 = off)",
        0, 127, 106));

    addParameter (gateResponseAmountCcParameter = new juce::AudioParameterInt (
        juce::ParameterID { "gateResponseAmountCc", 1 },
        "Gate Response Amount CC (0 = off)",
        0, 127, 107));
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    runRuntimeCatalogProbe (orchconductor::RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled());

    // A previously-imported narrative-lane library lives at
    // getNarrativeLibraryFile(); load it now so its lanes are available with
    // no per-project setup (same pattern as loadUserCombiPresetsFromUserLibrary).
    if (const auto narrativeLibrary = getNarrativeLibraryFile(); narrativeLibrary.existsAsFile())
    {
        const auto fileProbe = orchconductor::RuntimePresetSource::loadFromJsonFileIfEnabled (narrativeLibrary);

        if (fileProbe.wasLoaded() && runRuntimeCatalogProbe (fileProbe))
        {
            narrativeLibraryExternalFile = narrativeLibrary;
            narrativeLibrarySourceStatus =
                "Narrative library: " + juce::String (runtimePresetCatalog.getNarrativeLaneCount())
                + " lanes from " + narrativeLibrary.getFileName() + " (auto-loaded).";
        }
        else
        {
            narrativeLibrarySourceStatus =
                "Narrative library: " + narrativeLibrary.getFileName()
                + " could not be loaded (" + fileProbe.diagnosticMessage + ") - using built-in.";
        }
    }
#else
    runtimeJsonPresetProbeLoaded = false;
    runtimeJsonPresetProbeRequiresFallback = true;
    runtimeJsonPresetProbeDiagnostic = "Runtime JSON presets are disabled by ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS.";

    runtimePresetCatalogAuthorityProbeReady = false;
    runtimePresetCatalogAuthorityProbeRequiresFallback = true;
    runtimePresetCatalogAuthorityProbeHasExpectedFactoryShape = false;
    runtimePresetCatalogAuthorityProbeDiagnostic = "Runtime preset catalog authority probe is inactive because runtime JSON presets are disabled.";

    runtimeCatalogPayloadEquivalenceProbeRun = false;
    runtimeCatalogPayloadEquivalenceProbePassed = false;
    runtimeCatalogPayloadEquivalenceProbeBlockedByFallback = true;
    runtimeCatalogPayloadEquivalenceProbeDiagnostic =
        "Runtime catalog payload equivalence probe is inactive because runtime JSON presets are disabled.";

    runtimeCatalogCoverageAuditRun = false;
    runtimeCatalogCoverageAuditPassed = false;
    runtimeCatalogCoverageAuditBlockedByFallback = true;
    runtimeCatalogCoverageAuditDiagnostic =
        "Runtime catalog coverage audit is inactive because runtime JSON presets are disabled.";

    runtimeCatalogAuthorityTrialRun = false;
    runtimeCatalogAuthorityTrialPass = false;
    runtimeCatalogAuthorityTrialBlocked = true;
    runtimeCatalogAuthorityTrialDiagnostic =
        "Runtime catalog authority trial is inactive because runtime JSON presets are disabled.";
#endif

    loadUserCombiPresetsFromUserLibrary();

#if JUCE_DEBUG
    debugValidateFactoryCombiSectionCoverage();
#endif
}

OrchConductorAudioProcessor::~OrchConductorAudioProcessor()
{
}

bool OrchConductorAudioProcessor::runRuntimeCatalogProbe (const orchconductor::RuntimePresetSourceResult& runtimeJsonPresetProbe)
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    runtimeJsonPresetProbeLoaded = runtimeJsonPresetProbe.wasLoaded();
    runtimeJsonPresetProbeRequiresFallback = runtimeJsonPresetProbe.requiresHardcodedFallback();
    runtimeJsonPresetProbeDiagnostic = runtimeJsonPresetProbe.diagnosticMessage;

    const auto runtimePresetCatalogAuthorityProbe =
        OrchConductorRuntimePresetCatalog::createFromRuntimeSource (runtimeJsonPresetProbe);

    runtimePresetCatalogAuthorityProbeReady = runtimePresetCatalogAuthorityProbe.isReady();
    runtimePresetCatalogAuthorityProbeRequiresFallback = runtimePresetCatalogAuthorityProbe.requiresFallback();
    runtimePresetCatalogAuthorityProbeHasExpectedFactoryShape = runtimePresetCatalogAuthorityProbe.hasExpectedFactoryShape();
    runtimePresetCatalogAuthorityProbeDiagnostic = runtimePresetCatalogAuthorityProbe.getDiagnosticMessage();

    if (runtimePresetCatalogAuthorityProbe.isReady()
        && runtimePresetCatalogAuthorityProbe.hasExpectedFactoryShape()
        && ! runtimePresetCatalogAuthorityProbe.requiresFallback())
    {
        runtimeCatalogPayloadEquivalenceProbeRun = true;
        runtimeCatalogPayloadEquivalenceProbeBlockedByFallback = false;
        runtimeCatalogPayloadEquivalenceProbePassed =
            verifyRuntimeCatalogPayloadEquivalenceSentinels (runtimePresetCatalogAuthorityProbe);

        runtimeCatalogPayloadEquivalenceProbeDiagnostic =
            runtimeCatalogPayloadEquivalenceProbePassed
                ? "Runtime catalog payload equivalence probe passed for broadened sentinel hardcoded presets."
                : "Runtime catalog payload equivalence probe failed for broadened sentinel hardcoded presets.";

        runtimeCatalogCoverageAuditRun = true;
        runtimeCatalogCoverageAuditBlockedByFallback = false;
        runtimeCatalogCoverageAuditPassed =
            verifyRuntimeCatalogCoverageAudit (runtimePresetCatalogAuthorityProbe);

        runtimeCatalogCoverageAuditDiagnostic =
            runtimeCatalogCoverageAuditPassed
                ? "Runtime catalog coverage audit passed for expected factory catalog shape and narrative lanes."
                : "Runtime catalog coverage audit failed for expected factory catalog shape or narrative lanes.";
#if ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL
        runtimeCatalogAuthorityTrialRun = true;
        runtimeCatalogAuthorityTrialBlocked = false;
        runtimeCatalogAuthorityTrialPass =
            verifyRuntimeCatalogAuthorityTrialSentinels (runtimePresetCatalogAuthorityProbe);

        runtimeCatalogAuthorityTrialDiagnostic =
            runtimeCatalogAuthorityTrialPass
                ? "Runtime catalog authority trial diagnostic passed."
                : "Runtime catalog authority trial diagnostic failed: sentinel payloads do not match expected factory authority values.";
#else
        runtimeCatalogAuthorityTrialRun = false;
        runtimeCatalogAuthorityTrialPass = false;
        runtimeCatalogAuthorityTrialBlocked = true;
        runtimeCatalogAuthorityTrialDiagnostic =
            "Runtime catalog authority trial is disabled by ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL.";
#endif

#if ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL
        runtimePresetCatalogAuthorityActive =
            runtimeCatalogPayloadEquivalenceProbePassed
            && runtimeCatalogCoverageAuditPassed
            && runtimeCatalogAuthorityTrialPass;
#else
        runtimePresetCatalogAuthorityActive =
            runtimeCatalogPayloadEquivalenceProbePassed
            && runtimeCatalogCoverageAuditPassed;
#endif

        runtimePresetCatalogAuthorityStatus =
            runtimePresetCatalogAuthorityActive
                ? "Runtime preset catalog authority is active."
                : "Runtime preset catalog authority remains inactive because one or more runtime catalog validation gates failed.";

        // Adopt the catalog for its narrative lanes + labels whenever it is a
        // ready, factory-shaped source - NOT only when it is authoritative for
        // CC values. This is what lets an imported lane library take effect
        // even if its combi/section values diverge from the hardcoded tables
        // (those stay authoritative unless the separate authority trial is on).
        {
            const juce::SpinLock::ScopedLockType lock (runtimeCatalogLock);
            runtimePresetCatalog = runtimePresetCatalogAuthorityProbe;
        }

        return true;
    }

    runtimeCatalogPayloadEquivalenceProbeRun = false;
    runtimeCatalogPayloadEquivalenceProbePassed = false;
    runtimeCatalogPayloadEquivalenceProbeBlockedByFallback = true;
    runtimeCatalogPayloadEquivalenceProbeDiagnostic =
        "Runtime catalog payload equivalence probe blocked because runtime catalog requires fallback or is not source-backed.";

    runtimeCatalogCoverageAuditRun = false;
    runtimeCatalogCoverageAuditPassed = false;
    runtimeCatalogCoverageAuditBlockedByFallback = true;
    runtimeCatalogCoverageAuditDiagnostic =
        "Runtime catalog coverage audit blocked because runtime catalog requires fallback or is not source-backed.";

    runtimeCatalogAuthorityTrialRun = false;
    runtimeCatalogAuthorityTrialPass = false;
    runtimeCatalogAuthorityTrialBlocked = true;
    runtimeCatalogAuthorityTrialDiagnostic =
        "Runtime catalog authority trial diagnostic was blocked because runtime catalog requires fallback or is not source-backed.";

    return false;
#else
    juce::ignoreUnused (runtimeJsonPresetProbe);
    return false;
#endif
}

juce::File OrchConductorAudioProcessor::getNarrativeLibraryFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("OrchConductor")
        .getChildFile ("NarrativeLibrary.json");
}

bool OrchConductorAudioProcessor::importNarrativeLibraryFromFile (const juce::File& file)
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    const auto fileProbe = orchconductor::RuntimePresetSource::loadFromJsonFileIfEnabled (file);

    if (! fileProbe.wasLoaded())
    {
        narrativeLibrarySourceStatus = "Narrative library: import failed - " + fileProbe.diagnosticMessage;
        return false;
    }

    if (! runRuntimeCatalogProbe (fileProbe))
    {
        narrativeLibrarySourceStatus =
            "Narrative library: import rejected - file is not a complete factory-shaped library. Previous library kept.";
        return false;
    }

    // Keep a copy so it auto-loads on the next launch (same as the user-combi
    // library). A failed copy is non-fatal - the catalog is already live.
    const auto destination = getNarrativeLibraryFile();
    destination.getParentDirectory().createDirectory();
    const bool copied = file == destination ? true : file.copyFileTo (destination);

    narrativeLibraryExternalFile = destination;
    narrativeLibrarySourceStatus =
        "Narrative library: " + juce::String (runtimePresetCatalog.getNarrativeLaneCount())
        + " lanes from " + file.getFileName()
        + (copied ? juce::String (" (saved to library).") : juce::String (" (loaded; could not save copy)."));

    return true;
#else
    juce::ignoreUnused (file);
    return false;
#endif
}

bool OrchConductorAudioProcessor::exportNarrativeLibraryTemplateToFile (const juce::File& file) const
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    const auto embeddedJson = orchconductor::getEmbeddedFactoryJson();

    if (! embeddedJson.isValid())
        return false;

    const auto jsonText = juce::String::fromUTF8 (embeddedJson.data, embeddedJson.size);

    if (jsonText.isEmpty())
        return false;

    return file.replaceWithText (jsonText);
#else
    juce::ignoreUnused (file);
    return false;
#endif
}

juce::String OrchConductorAudioProcessor::getNarrativeLibrarySourceStatus() const
{
    return narrativeLibrarySourceStatus;
}

bool OrchConductorAudioProcessor::isNarrativeLibraryExternal() const
{
    return narrativeLibraryExternalFile != juce::File{};
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


bool OrchConductorAudioProcessor::wasRuntimeJsonPresetProbeLoaded() const
{
    return runtimeJsonPresetProbeLoaded;
}

bool OrchConductorAudioProcessor::doesRuntimeJsonPresetProbeRequireFallback() const
{
    return runtimeJsonPresetProbeRequiresFallback;
}

juce::String OrchConductorAudioProcessor::getRuntimeJsonPresetProbeDiagnostic() const
{
    return runtimeJsonPresetProbeDiagnostic;
}

bool OrchConductorAudioProcessor::wasRuntimePresetCatalogAuthorityProbeReady() const
{
    return runtimePresetCatalogAuthorityProbeReady;
}

bool OrchConductorAudioProcessor::doesRuntimePresetCatalogAuthorityProbeRequireFallback() const
{
    return runtimePresetCatalogAuthorityProbeRequiresFallback;
}

bool OrchConductorAudioProcessor::doesRuntimePresetCatalogAuthorityProbeHaveExpectedFactoryShape() const
{
    return runtimePresetCatalogAuthorityProbeHasExpectedFactoryShape;
}

juce::String OrchConductorAudioProcessor::getRuntimePresetCatalogAuthorityProbeDiagnostic() const
{
    return runtimePresetCatalogAuthorityProbeDiagnostic;
}

bool OrchConductorAudioProcessor::wasRuntimeCatalogPayloadEquivalenceProbeRun() const
{
    return runtimeCatalogPayloadEquivalenceProbeRun;
}

bool OrchConductorAudioProcessor::didRuntimeCatalogPayloadEquivalenceProbePass() const
{
    return runtimeCatalogPayloadEquivalenceProbePassed;
}

bool OrchConductorAudioProcessor::wasRuntimeCatalogPayloadEquivalenceProbeBlockedByFallback() const
{
    return runtimeCatalogPayloadEquivalenceProbeBlockedByFallback;
}

juce::String OrchConductorAudioProcessor::getRuntimeCatalogPayloadEquivalenceProbeDiagnostic() const
{
    return runtimeCatalogPayloadEquivalenceProbeDiagnostic;
}

bool OrchConductorAudioProcessor::wasRuntimeCatalogCoverageAuditRun() const
{
    return runtimeCatalogCoverageAuditRun;
}

bool OrchConductorAudioProcessor::didRuntimeCatalogCoverageAuditPass() const
{
    return runtimeCatalogCoverageAuditPassed;
}

bool OrchConductorAudioProcessor::wasRuntimeCatalogCoverageAuditBlockedByFallback() const
{
    return runtimeCatalogCoverageAuditBlockedByFallback;
}

juce::String OrchConductorAudioProcessor::getRuntimeCatalogCoverageAuditDiagnostic() const
{
    return runtimeCatalogCoverageAuditDiagnostic;
}

bool OrchConductorAudioProcessor::wasRuntimeCatalogAuthorityTrialRun() const
{
    return runtimeCatalogAuthorityTrialRun;
}

bool OrchConductorAudioProcessor::didRuntimeCatalogAuthorityTrialPass() const
{
    return runtimeCatalogAuthorityTrialPass;
}

bool OrchConductorAudioProcessor::wasRuntimeCatalogAuthorityTrialBlocked() const
{
    return runtimeCatalogAuthorityTrialBlocked;
}

juce::String OrchConductorAudioProcessor::getRuntimeCatalogAuthorityTrialDiagnostic() const
{
    return runtimeCatalogAuthorityTrialDiagnostic;
}

bool OrchConductorAudioProcessor::isRuntimePresetCatalogAuthorityActive() const
{
    return runtimePresetCatalogAuthorityActive;
}

juce::String OrchConductorAudioProcessor::getRuntimePresetCatalogAuthorityStatus() const
{
    if (runtimePresetCatalogAuthorityActive)
    {
        if (runtimeCatalogCoverageAuditRun && runtimeCatalogCoverageAuditPassed)
            return "Runtime JSON: Active | Factory catalog source-backed | Coverage audit passed";

        if (runtimeCatalogPayloadEquivalenceProbeRun && runtimeCatalogPayloadEquivalenceProbePassed)
            return "Runtime JSON: Active | Factory catalog source-backed | Payload probe passed";

        if (runtimePresetCatalogAuthorityProbeReady
            && runtimePresetCatalogAuthorityProbeHasExpectedFactoryShape
            && ! runtimePresetCatalogAuthorityProbeRequiresFallback)
            return "Runtime JSON: Active | Factory catalog source-backed";

        return "Runtime JSON: Active | Runtime catalog authority enabled";
    }

    if (runtimePresetCatalogAuthorityProbeRequiresFallback
        || runtimeCatalogCoverageAuditBlockedByFallback
        || runtimeCatalogPayloadEquivalenceProbeBlockedByFallback)
    {
        if (runtimePresetCatalogAuthorityProbeDiagnostic.isNotEmpty())
            return "Runtime JSON: Fallback required | " + runtimePresetCatalogAuthorityProbeDiagnostic;

        if (runtimeJsonPresetProbeDiagnostic.isNotEmpty())
            return "Runtime JSON: Fallback required | " + runtimeJsonPresetProbeDiagnostic;

        return "Runtime JSON: Fallback required | Hardcoded preset authority active";
    }

    if (runtimePresetCatalogAuthorityProbeDiagnostic.isNotEmpty())
        return "Runtime JSON: Inactive | " + runtimePresetCatalogAuthorityProbeDiagnostic;

    if (runtimeJsonPresetProbeDiagnostic.isNotEmpty())
        return "Runtime JSON: Inactive | " + runtimeJsonPresetProbeDiagnostic;

    return "Runtime JSON: Inactive | Hardcoded preset authority active";
}

int OrchConductorAudioProcessor::getDefaultMaxPlayersForCc (int ccNumber) const
{
    switch (ccNumber)
    {
        case 50: return 8; // Violin I
        case 51: return 6; // Violin II
        case 52: return 4; // Viola
        case 53: return 4; // Cello
        case 54: return 2; // Double Bass
        default: return 1;
    }
}

int OrchConductorAudioProcessor::getActivePlayersForValue (int value, int maxPlayers) const
{
    if (value <= 0 || maxPlayers <= 0)
        return 0;

    const auto scaled = static_cast<int> (std::round ((static_cast<double> (value) / 127.0) * static_cast<double> (maxPlayers)));
    return juce::jlimit (1, maxPlayers, scaled);
}

int OrchConductorAudioProcessor::getTotalActivePlayers() const
{
    int total = 0;

    for (int i = 0; i < getNumWoodwindsOutputRows(); ++i)
        total += getWoodwindsOutputRow (i).activePlayers;

    for (int i = 0; i < getNumBrassOutputRows(); ++i)
        total += getBrassOutputRow (i).activePlayers;

    for (int i = 0; i < getNumPercussionOutputRows(); ++i)
        total += getPercussionOutputRow (i).activePlayers;

    // Harp (CC49) and Piano (CC55) have no dedicated row/UI - they only
    // contribute when the active user combi overrides them.
    total += getActivePlayersForValue (getHarpCcValue(), 1);
    total += getActivePlayersForValue (getPianoCcValue(), 1);

    for (int i = 0; i < getNumOutputRows(); ++i)
        total += getOutputRow (i).activePlayers;

    return total;
}

int OrchConductorAudioProcessor::getHarpCcValue() const
{
    if (isCombiModeActive())
        return getCombiPresetValueForCc (reservedHarpCcNumber);

    if (isNarrativeScanDriving())
        return lastResolvedNarrativeHarpValue >= 0 ? lastResolvedNarrativeHarpValue : 0;

    return manualHarpValue >= 0 ? manualHarpValue : 0;
}

int OrchConductorAudioProcessor::getPianoCcValue() const
{
    if (isCombiModeActive())
        return getCombiPresetValueForCc (pianoCcNumber);

    if (isNarrativeScanDriving())
        return lastResolvedNarrativePianoValue >= 0 ? lastResolvedNarrativePianoValue : 0;

    return manualPianoValue >= 0 ? manualPianoValue : 0;
}

int OrchConductorAudioProcessor::getInstrumentSlotCount()
{
    return numWoodwindsRows + numBrassRows + numPercussionRows + numRows + 2;
}

juce::String OrchConductorAudioProcessor::getInstrumentSlotName (int slot) const
{
    if (slot < 0) return {};
    if (slot < numWoodwindsRows) return woodwindsInstrumentNames[slot];
    slot -= numWoodwindsRows;
    if (slot < numBrassRows) return brassInstrumentNames[slot];
    slot -= numBrassRows;
    if (slot < numPercussionRows) return percussionInstrumentNames[slot];
    slot -= numPercussionRows;
    if (slot < numRows) return instrumentNames[slot];
    slot -= numRows;
    if (slot == 0) return "Harp";
    if (slot == 1) return "Piano";
    return {};
}

int OrchConductorAudioProcessor::getInstrumentSlotCc (int slot) const
{
    if (slot < 0) return -1;
    if (slot < numWoodwindsRows) return woodwindsCcNumbers[slot];
    slot -= numWoodwindsRows;
    if (slot < numBrassRows) return brassCcNumbers[slot];
    slot -= numBrassRows;
    if (slot < numPercussionRows) return percussionCcNumbers[slot];
    slot -= numPercussionRows;
    if (slot < numRows) return ccNumbers[slot];
    slot -= numRows;
    if (slot == 0) return reservedHarpCcNumber;
    if (slot == 1) return pianoCcNumber;
    return -1;
}

juce::String OrchConductorAudioProcessor::getInstrumentSlotSectionName (int slot) const
{
    if (slot < 0) return {};
    if (slot < numWoodwindsRows) return "Woodwinds";
    slot -= numWoodwindsRows;
    if (slot < numBrassRows) return "Brass";
    slot -= numBrassRows;
    if (slot < numPercussionRows) return "Percussion";
    slot -= numPercussionRows;
    if (slot < numRows) return "Strings";
    slot -= numRows;
    if (slot == 0) return "Harp";
    if (slot == 1) return "Piano";
    return {};
}

int OrchConductorAudioProcessor::getInstrumentSlotCurrentValue (int slot) const
{
    if (slot < 0) return 0;
    if (slot < numWoodwindsRows) return getWoodwindsOutputRow (slot).value;
    slot -= numWoodwindsRows;
    if (slot < numBrassRows) return getBrassOutputRow (slot).value;
    slot -= numBrassRows;
    if (slot < numPercussionRows) return getPercussionOutputRow (slot).value;
    slot -= numPercussionRows;
    if (slot < numRows) return getOutputRow (slot).value;
    slot -= numRows;
    if (slot == 0) return getHarpCcValue();
    if (slot == 1) return getPianoCcValue();
    return 0;
}

int OrchConductorAudioProcessor::getCombiResolvedCcValue (int presetId, int ccNumber) const
{
    int value = getCombiPresetValueForCc (presetId, ccNumber);
    tryGetRuntimeCombiPresetValueForCc (presetId, ccNumber, value); // no-op unless runtime authority is active
    return value;
}

std::vector<orchconductor::PresetValue>
    OrchConductorAudioProcessor::getUserCombiExplicitCcValues (int presetId) const
{
    if (isUserCombiPresetId (presetId))
        if (const auto it = userCombiPresets.find (presetId); it != userCombiPresets.end())
            return it->second.explicitCcValues;

    return {};
}

int OrchConductorAudioProcessor::saveInstrumentGridAsUserCombi (
    const juce::String& name,
    const std::vector<orchconductor::PresetValue>& explicitValues,
    int existingUserCombiId)
{
    const bool updating = isUserCombiPresetId (existingUserCombiId)
                          && userCombiPresets.count (existingUserCombiId) != 0;

    const int presetId = updating ? existingUserCombiId : getNextAvailableUserCombiPresetId();

    if (presetId < 0)
        return -1;

    UserCombiPreset preset = updating ? userCombiPresets[existingUserCombiId] : UserCombiPreset {};

    if (name.isNotEmpty())
        preset.name = name;
    else if (preset.name.isEmpty())
        preset.name = "Grid Combi " + juce::String (presetId);

    preset.explicitCcValues.clear();

    for (const auto& value : explicitValues)
        if (value.isValid())
            preset.explicitCcValues.push_back (value);

    userCombiPresets[presetId] = preset;

    saveUserCombiPresetsToUserLibrary();

    return presetId;
}

std::vector<orchconductor::PresetValue>
    OrchConductorAudioProcessor::generateRandomGridCombi (GridRandomStyle style, juce::int64 seed) const
{
    juce::Random rng (seed);
    const int n = getInstrumentSlotCount();          // 43
    std::vector<int> v (static_cast<size_t> (n), 0);

    // Slot ranges (see getInstrumentSlot*): 0-11 WW, 12-22 BR, 23-35 PC,
    // 36-40 ST, 41 Harp, 42 Piano.
    auto ww = [] (int i) { return i >= 0  && i < 12; };
    auto br = [] (int i) { return i >= 12 && i < 23; };
    auto pc = [] (int i) { return i >= 23 && i < 36; };
    auto st = [] (int i) { return i >= 36 && i < 41; };
    constexpr int HARP = 41, PIANO = 42;

    float density  = 0.5f;
    float loudness = 0.5f;

    switch (style)
    {
        case GridRandomStyle::sparse:  density = rng.nextFloat() * 0.2f  + 0.12f; loudness = rng.nextFloat() * 0.3f + 0.2f;  break;
        case GridRandomStyle::tutti:   density = rng.nextFloat() * 0.15f + 0.82f; loudness = rng.nextFloat() * 0.2f + 0.78f; break;
        case GridRandomStyle::feature: density = rng.nextFloat() * 0.25f + 0.3f;  loudness = rng.nextFloat() * 0.3f + 0.4f;  break;
        case GridRandomStyle::balanced:
        default:                       density = rng.nextFloat() * 0.4f  + 0.35f; loudness = rng.nextFloat() * 0.4f + 0.35f; break;
    }

    const int reg = rng.nextInt (3) - 1;   // -1 low, 0 mid, +1 high

    auto valueFor = [&] (float role)
    {
        const float f = juce::jlimit (0.0f, 1.0f,
                                      loudness * 0.55f + role * 0.5f + (rng.nextFloat() - 0.5f) * 0.15f);
        return juce::jlimit (40, 127, juce::roundToInt (55.0f + f * 72.0f));
    };

    auto on = [&] (float prob) { return rng.nextFloat() < juce::jlimit (0.0f, 1.0f, prob); };

    // Strings - the backbone.
    {
        const bool anyStrings = style == GridRandomStyle::sparse ? on (0.6f) : on (0.92f);

        if (anyStrings)
        {
            const float upperProb = juce::jlimit (0.2f, 1.0f, (reg <= 0 ? 0.6f : 0.9f) + density * 0.3f);
            const float lowProb   = juce::jlimit (0.2f, 1.0f, (reg >= 0 ? 0.6f : 0.9f) + density * 0.3f);

            if (on (upperProb))       { v[36] = valueFor (0.8f); v[37] = valueFor (0.7f); }
            if (on (upperProb * 0.9f))  v[38] = valueFor (0.7f);
            if (on (lowProb))         { v[39] = valueFor (0.75f); v[40] = valueFor (0.7f); }
        }
    }

    // Woodwinds - pairs move together, solo colours are rarer.
    {
        const int  pairA[] = { 1, 3, 6, 9 };
        const int  pairB[] = { 2, 4, 7, 10 };
        const float bias[] = { 0.6f, 0.1f, -0.1f, -0.6f };   // >0 favours the high register

        for (int p = 0; p < 4; ++p)
            if (on (density * 0.9f + 0.1f + bias[p] * static_cast<float> (reg) * 0.25f))
            {
                v[pairA[p]] = valueFor (0.7f);
                v[pairB[p]] = valueFor (0.6f);
            }

        if (reg >= 0 && on (density * 0.35f)) v[0]  = valueFor (0.9f);   // Piccolo
        if (on (density * 0.3f))              v[5]  = valueFor (0.85f);  // English Horn
        if (reg <= 0 && on (density * 0.3f))  v[8]  = valueFor (0.75f);  // Bass Clarinet
        if (reg <  0 && on (density * 0.25f)) v[11] = valueFor (0.7f);   // Contrabassoon
    }

    // Brass - sectional.
    {
        if (on (density * 0.8f + 0.1f))
        {
            const int horns = rng.nextBool() ? 4 : 2;
            for (int i = 0; i < horns; ++i) v[12 + i] = valueFor (i == 0 ? 0.8f : 0.7f);
        }

        if (reg >= 0 && on (loudness * 0.6f + density * 0.3f))
        {
            const int trumpets = 2 + rng.nextInt (2);
            for (int i = 0; i < trumpets; ++i) v[16 + i] = valueFor (0.8f);
        }

        if (on ((reg <= 0 ? 0.5f : 0.3f) + loudness * 0.4f))
        {
            v[19] = valueFor (0.75f);
            v[20] = valueFor (0.7f);
            if (on (0.7f))                 v[21] = valueFor (0.7f);   // Bass Trombone
            if (reg < 0 || on (0.5f))      v[22] = valueFor (0.7f);   // Tuba
        }
    }

    // Percussion - Timpani with weight, one mallet colour, sparse unpitched.
    {
        if (on (loudness * 0.7f + density * 0.2f)) v[23] = valueFor (0.7f);

        if (on (density * 0.5f))
        {
            const int mallet = reg > 0 ? (rng.nextBool() ? 24 : 25)
                             : reg < 0 ? 26
                                       : (rng.nextBool() ? 26 : 27);
            v[mallet] = valueFor (0.75f);
        }

        if (loudness > 0.7f && on (0.4f)) v[28] = valueFor (0.6f);    // Tubular Bells

        if (loudness > 0.6f)
        {
            if (on (0.5f))                 v[29] = valueFor (0.6f);   // Bass Drum
            if (on (0.4f))                 v[31] = valueFor (0.7f);   // Cymbals
            if (on (0.25f))                v[35] = valueFor (0.6f);   // Triangle
            if (reg < 0 && on (0.3f))      v[33] = valueFor (0.6f);   // Tam-Tam
        }
        else if (on (0.15f))
        {
            v[35] = valueFor (0.5f);
        }
    }

    // Harp / Piano.
    if (on (reg >= 0 ? 0.45f : 0.3f))                                       v[HARP]  = valueFor (0.7f);
    if (style == GridRandomStyle::feature ? on (0.35f) : on (0.15f))        v[PIANO] = valueFor (0.7f);

    // Register wipe.
    if (reg < 0) for (int hi : { 0, 24, 25, 28, 35, 36, HARP }) if (rng.nextFloat() < 0.7f) v[hi] = 0;
    if (reg > 0) for (int lo : { 8, 11, 21, 22, 26, 29, 33, 40 }) if (rng.nextFloat() < 0.7f) v[lo] = 0;

    // Feature: boost one active section, thin the rest.
    if (style == GridRandomStyle::feature)
    {
        const int feat = rng.nextInt (4);   // 0 WW, 1 BR, 2 ST, 3 PC

        for (int i = 0; i < n; ++i)
        {
            const bool inFeat = (feat == 0 && ww (i)) || (feat == 1 && br (i))
                             || (feat == 2 && st (i)) || (feat == 3 && pc (i));

            if (inFeat && v[i] == 0 && on (0.5f))      v[i] = valueFor (0.85f);
            else if (inFeat && v[i] > 0)               v[i] = juce::jmin (127, v[i] + 15);
            else if (! inFeat && v[i] > 0 && on (0.4f)) v[i] = 0;
        }
    }

    // Never silence.
    if (std::none_of (v.begin(), v.end(), [] (int x) { return x > 0; }))
    {
        v[36] = 100; v[37] = 100; v[38] = 90; v[39] = 90; v[40] = 80;
    }

    std::vector<orchconductor::PresetValue> out;
    out.reserve (static_cast<size_t> (n));

    for (int i = 0; i < n; ++i)
    {
        orchconductor::PresetValue pv;
        pv.ccNumber = getInstrumentSlotCc (i);
        pv.value = juce::jlimit (0, 127, v[static_cast<size_t> (i)]);
        out.push_back (pv);
    }

    return out;
}
void OrchConductorAudioProcessor::prepareToPlay (double, int)
{
    lastResolvedNarrativePointIndex = -1;
    lastResolvedNarrativeCombiId = -1;
    lastResolvedNarrativeHarpValue = -1;
    lastResolvedNarrativePianoValue = -1;
    lastResolvedGateResponseMode = -1;
    lastResolvedGateResponseAmount = -1;
    lastSentGateResponseMode = -1;
    lastSentGateResponseAmount = -1;
    pendingFieldSelectIndex = -1;
    lastSentFieldSelectIndex = -1;
    explicitSendPresetRequested = false;
}

void OrchConductorAudioProcessor::releaseResources()
{
}

bool OrchConductorAudioProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
    return true;
}

void OrchConductorAudioProcessor::syncAutomatedParameters()
{
    if (combiPresetParameter != nullptr)
    {
        const int automatedCombiPresetId = combiPresetParameter->get();

        if (automatedCombiPresetId != combiPresetId)
            setCombiPresetId (automatedCombiPresetId);
    }

    if (woodwindsPresetParameter != nullptr)
    {
        const int value = woodwindsPresetParameter->get();

        if (value != woodwindsPresetId)
            setSectionPresetId (Section::woodwinds, value);
    }

    if (brassPresetParameter != nullptr)
    {
        const int value = brassPresetParameter->get();

        if (value != brassPresetId)
            setSectionPresetId (Section::brass, value);
    }

    if (percussionPresetParameter != nullptr)
    {
        const int value = percussionPresetParameter->get();

        if (value != percussionPresetId)
            setSectionPresetId (Section::percussion, value);
    }

    if (stringsPresetParameter != nullptr)
    {
        const int value = stringsPresetParameter->get();

        if (value != stringsPresetId)
            setSectionPresetId (Section::strings, value);
    }

    if (sendOnPresetChangeParameter != nullptr)
    {
        const bool value = sendOnPresetChangeParameter->get();

        if (value != sendOnPresetChange)
            sendOnPresetChange = value;
    }

    if (inputPassthroughParameter != nullptr)
    {
        const auto value = static_cast<InputPassthroughMode> (
            juce::jlimit (0, 2, inputPassthroughParameter->getIndex()));

        if (value != inputPassthroughMode)
            inputPassthroughMode = value;
    }

    if (authorityModeParameter != nullptr)
    {
        const auto value = static_cast<AuthorityMode> (authorityModeParameter->getIndex());

        if (value != authorityMode)
            setAuthorityMode (value);
    }

    if (narrativeLaneParameter != nullptr)
    {
        const int value = narrativeLaneParameter->get();

        if (value != narrativeLaneIndex)
            narrativeLaneIndex = value;
    }

    if (narrativePositionParameter != nullptr)
    {
        const double value = static_cast<double> (narrativePositionParameter->get());

        if (value != narrativePosition)
            narrativePosition = value;
    }
}
OrchConductorAudioProcessor::AuthorityMode OrchConductorAudioProcessor::getAuthorityMode() const
{
    return authorityMode;
}

void OrchConductorAudioProcessor::setAuthorityMode (AuthorityMode mode)
{
    if (mode != authorityMode)
    {
        // Force a fresh resolve (and send) the next time narrative scan runs,
        // regardless of which path (UI / automation / CC) changed the mode.
        lastResolvedNarrativePointIndex = -1;
        lastResolvedNarrativeCombiId = -1;
        lastResolvedNarrativeHarpValue = -1;
        lastResolvedNarrativePianoValue = -1;
        lastResolvedGateResponseMode = -1;
        lastResolvedGateResponseAmount = -1;
        pendingFieldSelectIndex = -1;
        lastSentFieldSelectIndex = -1;
    }

    authorityMode = mode;

    if (authorityModeParameter != nullptr
        && authorityModeParameter->getIndex() != static_cast<int> (mode))
        *authorityModeParameter = static_cast<int> (mode);
}

int OrchConductorAudioProcessor::getNarrativeLaneIndex() const
{
    return narrativeLaneIndex;
}

void OrchConductorAudioProcessor::setNarrativeLaneIndex (int laneIndex)
{
    const int clamped = juce::jlimit (minNarrativeLaneParameterId, maxNarrativeLaneParameterId, laneIndex);

    if (clamped != narrativeLaneIndex)
    {
        // Fresh nearest-point lookup for the new lane; keep the last combi id so
        // an unchanged resolved combi still suppresses a redundant send.
        lastResolvedNarrativePointIndex = -1;
    }

    narrativeLaneIndex = clamped;

    if (narrativeLaneParameter != nullptr && narrativeLaneParameter->get() != clamped)
        *narrativeLaneParameter = clamped;
}

void OrchConductorAudioProcessor::requestNarrativeReresolve()
{
    // Force the next processBlock to re-resolve the current lane from scratch
    // and send, even if it lands on the same combi id (used after the Lane
    // Maker edits the library or the editor re-points the lane).
    lastResolvedNarrativePointIndex = -1;
    lastResolvedNarrativeCombiId = -1;
    lastResolvedNarrativeHarpValue = -1;
    lastResolvedNarrativePianoValue = -1;
    lastResolvedGateResponseMode = -1;
    lastResolvedGateResponseAmount = -1;

    if (authorityMode == AuthorityMode::narrativeScan)
        sendPresetRequested = true;
}

void OrchConductorAudioProcessor::setGateResponseManualEnabled (bool shouldEnable)
{
    if (gateResponseManualEnabled == shouldEnable)
        return;

    gateResponseManualEnabled = shouldEnable;
    sendPresetRequested = true;   // emit (or neutralize) the bridge CCs next block
}

bool OrchConductorAudioProcessor::isGateResponseManualEnabled() const
{
    return gateResponseManualEnabled;
}

void OrchConductorAudioProcessor::setGateResponseManualMode (int mode)
{
    const int clamped = juce::jlimit (0, 127, mode);

    if (gateResponseManualMode == clamped)
        return;

    gateResponseManualMode = clamped;

    if (gateResponseManualEnabled)
        sendPresetRequested = true;
}

int OrchConductorAudioProcessor::getGateResponseManualMode() const
{
    return gateResponseManualMode;
}

void OrchConductorAudioProcessor::setGateResponseManualAmount (int amount)
{
    const int clamped = juce::jlimit (0, 127, amount);

    if (gateResponseManualAmount == clamped)
        return;

    gateResponseManualAmount = clamped;

    if (gateResponseManualEnabled)
        sendPresetRequested = true;
}

int OrchConductorAudioProcessor::getGateResponseManualAmount() const
{
    return gateResponseManualAmount;
}

void OrchConductorAudioProcessor::shuffleGateResponseMode()
{
    // 1..127 - value 0 is "start of piece / no chapter yet" on the OrchGate
    // side, so never land there deliberately.
    gateResponseManualMode = 1 + gateResponseShuffleRng.nextInt (127);
    gateResponseManualEnabled = true;
    sendPresetRequested = true;
}

int OrchConductorAudioProcessor::getEffectiveGateResponseMode() const
{
    if (gateResponseManualEnabled)
        return gateResponseManualMode;

    if (isNarrativeScanDriving() && lastResolvedGateResponseMode >= 0)
        return lastResolvedGateResponseMode;

    return -1;
}

int OrchConductorAudioProcessor::getEffectiveGateResponseAmount() const
{
    if (gateResponseManualEnabled)
        return gateResponseManualAmount;

    if (isNarrativeScanDriving() && lastResolvedGateResponseAmount >= 0)
        return lastResolvedGateResponseAmount;

    return -1;
}

double OrchConductorAudioProcessor::getNarrativePosition() const
{
    return narrativePosition;
}

void OrchConductorAudioProcessor::setNarrativePosition (double position)
{
    const double clamped = juce::jlimit (0.0, 1.0, position);

    narrativePosition = clamped;

    if (narrativePositionParameter != nullptr)
        *narrativePositionParameter = static_cast<float> (clamped);
}

void OrchConductorAudioProcessor::applyNarrativeControlCcInput (const juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (! message.isController())
            continue;

        const int cc = message.getControllerNumber();
        const int value = juce::jlimit (0, 127, message.getControllerValue());

        if (cc == narrativePositionCcNumber)
        {
            const double position = static_cast<double> (value) / 127.0;

            if (std::abs (position - narrativePosition) > 0.0001)
                setNarrativePosition (position);
        }
        else if (cc == narrativeLaneCcNumber)
        {
            const int laneCount = getNarrativeLaneCount();
            const int maxLane = laneCount > 0 ? juce::jmin (laneCount - 1, maxNarrativeLaneParameterId)
                                              : maxNarrativeLaneParameterId;
            const int lane = juce::jlimit (0, maxLane, value);

            if (lane != narrativeLaneIndex)
                setNarrativeLaneIndex (lane);
        }
        else if (cc == authorityModeCcNumber)
        {
            const auto mode = value < 43  ? AuthorityMode::manualSections
                            : value < 86  ? AuthorityMode::combiPreset
                                          : AuthorityMode::narrativeScan;

            if (mode != authorityMode)
                setAuthorityMode (mode);
        }
    }
}

void OrchConductorAudioProcessor::updateNarrativeScanResolution()
{
    // runtimePresetCatalog can be replaced whole by importNarrativeLibraryFromFile()
    // on the message thread - hold the lock across every read of it here.
    const juce::SpinLock::ScopedLockType lock (runtimeCatalogLock);

    // Narrative lanes live only in the loaded runtime catalog. If the catalog
    // is the fallback (no lanes) the resolver returns an invalid selection and
    // nothing is sent.
    const auto selection = OrchConductorNarrativeScanResolver::resolve (
        runtimePresetCatalog,
        narrativeLaneIndex,
        narrativePosition,
        lastResolvedNarrativePointIndex);

    if (! selection.isValid)
        return;

    const int previousPointIndex = lastResolvedNarrativePointIndex;
    lastResolvedNarrativePointIndex = selection.pointIndex;

    if (selection.pointIndex != previousPointIndex)
    {
        const int fieldIndex =
            runtimePresetCatalog.getNarrativeLanePointPitchFieldIndex (narrativeLaneIndex, selection.pointIndex);

        if (fieldIndex >= 0 && fieldIndex != lastSentFieldSelectIndex)
            pendingFieldSelectIndex = fieldIndex;

        const int harpValue =
            runtimePresetCatalog.getNarrativeLanePointHarpValue (narrativeLaneIndex, selection.pointIndex);
        const int pianoValue =
            runtimePresetCatalog.getNarrativeLanePointPianoValue (narrativeLaneIndex, selection.pointIndex);

        if (harpValue != lastResolvedNarrativeHarpValue || pianoValue != lastResolvedNarrativePianoValue)
        {
            lastResolvedNarrativeHarpValue = harpValue;
            lastResolvedNarrativePianoValue = pianoValue;
            sendPresetRequested = true; // harp/piano ride in the combi payload send
        }

        const int gateResponseMode =
            runtimePresetCatalog.getNarrativeLanePointGateResponseMode (narrativeLaneIndex, selection.pointIndex);
        const int gateResponseAmount =
            runtimePresetCatalog.getNarrativeLanePointGateResponseAmount (narrativeLaneIndex, selection.pointIndex);

        if (gateResponseMode != lastResolvedGateResponseMode
            || gateResponseAmount != lastResolvedGateResponseAmount)
        {
            lastResolvedGateResponseMode = gateResponseMode;
            lastResolvedGateResponseAmount = gateResponseAmount;
            sendPresetRequested = true; // bridge CCs emitted alongside the payload
        }
    }

    if (selection.combiId != lastResolvedNarrativeCombiId)
    {
        lastResolvedNarrativeCombiId = selection.combiId;
        sendPresetRequested = true;
    }
}

int OrchConductorAudioProcessor::getLastSentFieldSelectIndex() const
{
    return lastSentFieldSelectIndex;
}

juce::String OrchConductorAudioProcessor::getRuntimeCatalogSectionId (Section section)
{
    switch (section)
    {
        case Section::woodwinds:  return "woodwinds";
        case Section::brass:      return "brass";
        case Section::percussion: return "percussion";
        case Section::strings:    return "strings";

    }

    return {};
}
bool OrchConductorAudioProcessor::tryGetRuntimeSectionPresetValueForCc (Section section,
                                                                        int presetId,
                                                                        int ccNumber,
                                                                        int& value) const
{
    if (! runtimePresetCatalogAuthorityActive)
        return false;

    const auto sectionId = getRuntimeCatalogSectionId (section);

    if (sectionId.isEmpty())
        return false;

    const juce::SpinLock::ScopedLockType lock (runtimeCatalogLock);

    const int valueCount = runtimePresetCatalog.getSectionPresetValueCount (sectionId, presetId);

    for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex)
    {
        const auto runtimeValue = runtimePresetCatalog.getSectionPresetValue (sectionId, presetId, valueIndex);

        if (tryAssignRuntimeCatalogValueForCc (runtimeValue, ccNumber, value))
            return true;
    }

    return false;
}

bool OrchConductorAudioProcessor::tryGetRuntimeCombiPresetValueForCc (int presetId,
                                                                      int ccNumber,
                                                                      int& value) const
{
    if (! runtimePresetCatalogAuthorityActive)
        return false;

    const juce::SpinLock::ScopedLockType lock (runtimeCatalogLock);

    const int valueCount = runtimePresetCatalog.getCombiPresetValueCount (presetId);

    for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex)
    {
        const auto runtimeValue = runtimePresetCatalog.getCombiPresetValue (presetId, valueIndex);

        if (tryAssignRuntimeCatalogValueForCc (runtimeValue, ccNumber, value))
            return true;
    }

    return false;
}
void OrchConductorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    applyNarrativeControlCcInput (midiMessages);

    // Decide what of the input stream survives (after the bridge CCs are read).
    // OC's own CC20-54 / CC105 are added later and are unaffected by this.
    switch (inputPassthroughMode)
    {
        case InputPassthroughMode::off:
            midiMessages.clear();
            break;

        case InputPassthroughMode::controlCcs:
        {
            juce::MidiBuffer kept;

            for (const auto metadata : midiMessages)
            {
                const auto message = metadata.getMessage();

                if (message.isController()
                    && message.getControllerNumber() >= controlCcPassthroughFloor)
                    kept.addEvent (message, metadata.samplePosition);
            }

            midiMessages.swapWith (kept);
            break;
        }

        case InputPassthroughMode::all:
            break;
    }

    syncAutomatedParameters();

    if (authorityMode == AuthorityMode::narrativeScan)
        updateNarrativeScanResolution();

    // See wasHostPlaying's declaration: a stopped-transport "Send Preset"
    // can queue the CC dump without the percussion Arbiter chain ever
    // actually seeing it. Force one guaranteed-fresh full resend on the
    // stopped->playing transition, when the engine is unquestionably
    // pumping every track.
    bool hostIsPlaying = false;

    if (auto* transport = getPlayHead())
        if (const auto position = transport->getPosition())
            hostIsPlaying = position->getIsPlaying();

    if (hostIsPlaying && ! wasHostPlaying)
        requestSendPreset();

    wasHostPlaying = hostIsPlaying;

    const bool shouldSendAllOff = consumeSendAllOffRequest();
    const bool shouldSendPreset = consumeSendPresetRequest();
    const bool explicitSend = explicitSendPresetRequested;
    explicitSendPresetRequested = false;

    // Field-select CC (OrchNoteFilter pitch field). Emitted when the resolved
    // lane point changes, and re-emitted on an explicit "Send Current Presets"
    // (the current point's own field, so it can't push a stale value) so a
    // late-joining OrchNoteFilter can be brought current.
    {
        int fieldToSend = pendingFieldSelectIndex;

        if (fieldToSend < 0
            && explicitSend
            && authorityMode == AuthorityMode::narrativeScan
            && lastResolvedNarrativePointIndex >= 0)
        {
            const int currentPointField = [this]
            {
                const juce::SpinLock::ScopedLockType lock (runtimeCatalogLock);
                return runtimePresetCatalog.getNarrativeLanePointPitchFieldIndex (
                    narrativeLaneIndex, lastResolvedNarrativePointIndex);
            }();

            if (currentPointField >= 0)
                fieldToSend = currentPointField;
        }

        if (fieldToSend >= 0)
        {
            const int cc = fieldSelectCcParameter != nullptr ? fieldSelectCcParameter->get() : 105;

            if (cc > 0)
            {
                const int ccValue = juce::jlimit (0, 127,
                    juce::roundToInt (static_cast<double> (fieldToSend) / maxPitchFieldIndex * 127.0));
                midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, ccValue), 0);
            }

            lastSentFieldSelectIndex = fieldToSend;
            pendingFieldSelectIndex = -1;
        }
    }

    // OrchGate response bridge (CC 106 mode / CC 107 amount). Broadcast when
    // the effective mode/amount changes, re-broadcast on an explicit "Send
    // Current Presets" (so a late-joining OrchGate is brought current), and
    // forced to amount 0 on "Send All Off" (every OrchGate then falls back to
    // its own literal CC Invert / Threshold / Participation knobs).
    if (shouldSendAllOff || shouldSendPreset)
    {
        const int modeCc = gateResponseModeCcParameter != nullptr
            ? gateResponseModeCcParameter->get() : gateResponseModeCcDefault;
        const int amountCc = gateResponseAmountCcParameter != nullptr
            ? gateResponseAmountCcParameter->get() : gateResponseAmountCcDefault;

        const bool forceResend = explicitSend || shouldSendAllOff;

        int effMode = getEffectiveGateResponseMode();
        int effAmount = getEffectiveGateResponseAmount();

        if (shouldSendAllOff)
        {
            // Neutralise the bridge, but only if it was ever armed - a rig that
            // never heard CC106/107 gets no spurious "amount 0" broadcast.
            effMode = -1;
            effAmount = (lastSentGateResponseMode >= 0 || lastSentGateResponseAmount > 0) ? 0 : -1;
        }
        else
        {
            // Driving source went away (panel disabled / lane point clears it)
            // after we broadcast a non-zero amount: emit 0 once so every
            // OrchGate returns to its own literal knobs.
            if (effAmount < 0 && lastSentGateResponseAmount > 0)
                effAmount = 0;
        }

        if (effMode >= 0 && modeCc > 0
            && (forceResend || effMode != lastSentGateResponseMode))
        {
            midiMessages.addEvent (
                juce::MidiMessage::controllerEvent (1, modeCc, juce::jlimit (0, 127, effMode)), 0);
            lastSentGateResponseMode = effMode;
        }

        if (effAmount >= 0 && amountCc > 0
            && (forceResend || effAmount != lastSentGateResponseAmount))
        {
            midiMessages.addEvent (
                juce::MidiMessage::controllerEvent (1, amountCc, juce::jlimit (0, 127, effAmount)), 0);
            lastSentGateResponseAmount = effAmount;
        }
    }

    if (! shouldSendAllOff && ! shouldSendPreset)
        return;

    // Narrative scan borrows the combi CC payload path but resolves its own
    // combi id instead of touching combiPresetId / the combiPreset parameter.
    const bool useCombi = ! shouldSendAllOff && isEffectiveCombiModeActive();

    for (int i = 0; i < numWoodwindsRows; ++i)
    {
        const int cc = woodwindsCcNumbers[i];

        int value = shouldSendAllOff ? 0
                  : useCombi        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), cc)
                                    : getWoodwindsPresetValueForIndex (i);

        if (! shouldSendAllOff)
        {
            if (useCombi)
                tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), cc, value);

            if (! useCombi)
                tryGetRuntimeSectionPresetValueForCc (Section::woodwinds, woodwindsPresetId, cc, value);
        }

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    for (int i = 0; i < numBrassRows; ++i)
    {
        const int cc = brassCcNumbers[i];

        int value = shouldSendAllOff ? 0
                  : useCombi        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), cc)
                                    : getBrassPresetValueForIndex (i);

        if (! shouldSendAllOff)
        {
            if (useCombi)
                tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), cc, value);

            if (! useCombi)
                tryGetRuntimeSectionPresetValueForCc (Section::brass, brassPresetId, cc, value);
        }

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    for (int i = 0; i < numPercussionRows; ++i)
    {
        const int cc = percussionCcNumbers[i];

        int value = shouldSendAllOff ? 0
                  : useCombi        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), cc)
                                    : getPercussionPresetValueForIndex (i);

        if (! shouldSendAllOff)
        {
            if (useCombi)
                tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), cc, value);

            if (! useCombi)
                tryGetRuntimeSectionPresetValueForCc (Section::percussion, percussionPresetId, cc, value);
        }

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    // Harp (CC49) and Piano (CC55) carry a value from, in precedence order:
    // the manual Harp/Piano sliders (Manual Sections mode only), the resolved
    // narrative lane point (Narrative Scan mode), or a user combi's
    // harpValue/pianoValue (Combi mode). Every factory combi still resolves
    // both to 0.
    int harpValue = (! shouldSendAllOff && useCombi)
                        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), reservedHarpCcNumber)
                        : 0;

    if (! shouldSendAllOff && useCombi)
        tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), reservedHarpCcNumber, harpValue);

    if (! shouldSendAllOff && isNarrativeScanDriving() && lastResolvedNarrativeHarpValue >= 0)
        harpValue = lastResolvedNarrativeHarpValue;

    if (! shouldSendAllOff && ! useCombi && manualHarpValue >= 0)
        harpValue = manualHarpValue;

    midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, reservedHarpCcNumber, harpValue), 0);

    int pianoValue = (! shouldSendAllOff && useCombi)
                          ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), pianoCcNumber)
                          : 0;

    if (! shouldSendAllOff && useCombi)
        tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), pianoCcNumber, pianoValue);

    if (! shouldSendAllOff && isNarrativeScanDriving() && lastResolvedNarrativePianoValue >= 0)
        pianoValue = lastResolvedNarrativePianoValue;

    if (! shouldSendAllOff && ! useCombi && manualPianoValue >= 0)
        pianoValue = manualPianoValue;

    midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, pianoCcNumber, pianoValue), 0);

    for (int i = 0; i < numRows; ++i)
    {
        const int cc = ccNumbers[i];

        int value = shouldSendAllOff ? 0
                  : useCombi        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), cc)
                                    : getPresetValueForIndex (i);

        if (! shouldSendAllOff)
        {
            if (useCombi)
                tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), cc, value);

            if (! useCombi)
                tryGetRuntimeSectionPresetValueForCc (Section::strings, stringsPresetId, cc, value);
        }

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

    stream.writeInt (7);
    stream.writeInt (combiPresetId);
    stream.writeInt (woodwindsPresetId);
    stream.writeInt (brassPresetId);
    stream.writeInt (percussionPresetId);
    stream.writeInt (stringsPresetId);
    stream.writeBool (sendOnPresetChange);

    stream.writeInt (static_cast<int> (userCombiPresets.size()));

    for (const auto& [id, preset] : userCombiPresets)
    {
        stream.writeInt (id);
        stream.writeString (preset.name);
        stream.writeInt (preset.woodwindsPresetId);
        stream.writeInt (preset.brassPresetId);
        stream.writeInt (preset.percussionPresetId);
        stream.writeInt (preset.stringsPresetId);
        stream.writeInt (preset.harpValue);
        stream.writeInt (preset.pianoValue);

        stream.writeInt (static_cast<int> (preset.explicitCcValues.size()));

        for (const auto& explicitValue : preset.explicitCcValues)
        {
            stream.writeInt (explicitValue.ccNumber);
            stream.writeInt (explicitValue.value);
        }
    }

    // v3: narrative-scan authority state.
    stream.writeInt (static_cast<int> (authorityMode));
    stream.writeInt (narrativeLaneIndex);
    stream.writeDouble (narrativePosition);

    // v3 (appended): input-passthrough. A legacy bool slot (kept so an older
    // build reads something sane) followed by the real 3-way mode int.
    stream.writeBool (inputPassthroughMode != InputPassthroughMode::off);
    stream.writeInt (static_cast<int> (inputPassthroughMode));

    // v6: manual Harp (CC49) / Piano (CC55) values (-1 = Off).
    stream.writeInt (manualHarpValue);
    stream.writeInt (manualPianoValue);

    // v7: OrchGate response bridge live-panel state.
    stream.writeBool (gateResponseManualEnabled);
    stream.writeInt (gateResponseManualMode);
    stream.writeInt (gateResponseManualAmount);
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

    if (firstInt >= 1 && firstInt <= 7)
    {
        const auto version = firstInt;
        const auto combi = stream.readInt();
        const auto woodwinds = stream.readInt();
        const auto brass = stream.readInt();
        const auto percussion = stream.readInt();
        const auto strings = stream.readInt();

        auto restoredSendOnPresetChange = sendOnPresetChange;

        if (! stream.isExhausted())
            restoredSendOnPresetChange = stream.readBool();

        std::map<int, UserCombiPreset> restoredUserCombiPresets;

        if (version >= 2 && ! stream.isExhausted())
        {
            const auto count = stream.readInt();

            for (int i = 0; i < count && ! stream.isExhausted(); ++i)
            {
                const auto id = stream.readInt();
                UserCombiPreset preset;
                preset.name = stream.readString();
                preset.woodwindsPresetId = stream.readInt();
                preset.brassPresetId = stream.readInt();
                preset.percussionPresetId = stream.readInt();
                preset.stringsPresetId = stream.readInt();

                if (version >= 4 && ! stream.isExhausted())
                {
                    preset.harpValue = stream.readInt();
                    preset.pianoValue = stream.readInt();
                }

                if (version >= 5 && ! stream.isExhausted())
                {
                    const auto explicitCount = stream.readInt();

                    for (int v = 0; v < explicitCount && ! stream.isExhausted(); ++v)
                    {
                        orchconductor::PresetValue explicitValue;
                        explicitValue.ccNumber = stream.readInt();
                        explicitValue.value = stream.readInt();

                        if (explicitValue.isValid())
                            preset.explicitCcValues.push_back (explicitValue);
                    }
                }

                if (id >= firstUserCombiPresetId && id <= maxCombiPresetParameterId)
                    restoredUserCombiPresets[id] = preset;
            }
        }

        const auto previousSendOnPresetChange = sendOnPresetChange;
        sendOnPresetChange = false;

        userCombiPresets = std::move (restoredUserCombiPresets);

        setSectionPresetId (Section::woodwinds, woodwinds);
        setSectionPresetId (Section::brass, brass);
        setSectionPresetId (Section::percussion, percussion);
        setSectionPresetId (Section::strings, strings);
        setCombiPresetId (combi);

        sendOnPresetChange = previousSendOnPresetChange;
        setSendOnPresetChange (restoredSendOnPresetChange);

        if (version >= 3 && ! stream.isExhausted())
        {
            const auto restoredAuthorityMode = stream.readInt();
            const auto restoredNarrativeLane = stream.readInt();
            const auto restoredNarrativePosition = stream.readDouble();

            if (restoredAuthorityMode >= static_cast<int> (AuthorityMode::manualSections)
                && restoredAuthorityMode <= static_cast<int> (AuthorityMode::narrativeScan))
                setAuthorityMode (static_cast<AuthorityMode> (restoredAuthorityMode));

            setNarrativeLaneIndex (restoredNarrativeLane);
            setNarrativePosition (restoredNarrativePosition);
        }

        if (version >= 3 && ! stream.isExhausted())
        {
            const bool legacyPass = stream.readBool();

            if (! stream.isExhausted())
            {
                const int mode = stream.readInt();

                if (mode >= 0 && mode <= 2)
                    setInputPassthroughMode (static_cast<InputPassthroughMode> (mode));
            }
            else
            {
                // Older save: only the legacy bool was written.
                setInputPassthroughMode (legacyPass ? InputPassthroughMode::all
                                                    : InputPassthroughMode::off);
            }
        }

        if (version >= 6 && ! stream.isExhausted())
        {
            // Set the members directly (not via the setters) so restoring a
            // project doesn't queue a send request.
            const int restoredHarp = stream.readInt();
            manualHarpValue = (restoredHarp >= 0 && restoredHarp <= 127) ? restoredHarp : -1;

            if (! stream.isExhausted())
            {
                const int restoredPiano = stream.readInt();
                manualPianoValue = (restoredPiano >= 0 && restoredPiano <= 127) ? restoredPiano : -1;
            }
        }

        if (version >= 7 && ! stream.isExhausted())
        {
            // Members set directly (no send queued on project restore); the
            // bridge re-broadcasts on the next Send Preset / transport start.
            gateResponseManualEnabled = stream.readBool();

            if (! stream.isExhausted())
                gateResponseManualMode = juce::jlimit (0, 127, stream.readInt());

            if (! stream.isExhausted())
                gateResponseManualAmount = juce::jlimit (0, 127, stream.readInt());
        }

        return;
    }

    if (firstInt >= minStringsPresetId && firstInt <= maxStringsPresetId)
        stringsPresetId = firstInt;

    if (! stream.isExhausted())
        setSendOnPresetChange (stream.readBool());
}

int OrchConductorAudioProcessor::getCombiPresetId() const
{
    return combiPresetId;
}

void OrchConductorAudioProcessor::setCombiPresetId (int presetId)
{
    if (presetId < minCombiPresetId || presetId > getMaxCombiPresetId())
        return;

    const bool changed = combiPresetId != presetId;

    combiPresetId = presetId;

    int mappedWoodwinds = woodwindsPresetId;
    int mappedBrass = brassPresetId;
    int mappedPercussion = percussionPresetId;
    int mappedStrings = stringsPresetId;

    if (! runtimePresetCatalogAuthorityActive
        && getSectionPresetIdsForCombiPreset (presetId,
                                              mappedWoodwinds,
                                              mappedBrass,
                                              mappedPercussion,
                                              mappedStrings))
    {
        woodwindsPresetId = mappedWoodwinds;
        brassPresetId = mappedBrass;
        percussionPresetId = mappedPercussion;
        stringsPresetId = mappedStrings;

        if (woodwindsPresetParameter != nullptr && woodwindsPresetParameter->get() != woodwindsPresetId)
            *woodwindsPresetParameter = woodwindsPresetId;

        if (brassPresetParameter != nullptr && brassPresetParameter->get() != brassPresetId)
            *brassPresetParameter = brassPresetId;

        if (percussionPresetParameter != nullptr && percussionPresetParameter->get() != percussionPresetId)
            *percussionPresetParameter = percussionPresetId;

        if (stringsPresetParameter != nullptr && stringsPresetParameter->get() != stringsPresetId)
            *stringsPresetParameter = stringsPresetId;
    }

    if (combiPresetParameter != nullptr && combiPresetParameter->get() != presetId)
        *combiPresetParameter = presetId;

    if (changed && sendOnPresetChange)
        requestSendPreset();
}

bool OrchConductorAudioProcessor::isCombiModeActive() const
{
    return combiPresetId != static_cast<int> (CombiPreset::manualSections);
}

bool OrchConductorAudioProcessor::isNarrativeScanDriving() const
{
    return authorityMode == AuthorityMode::narrativeScan
        && lastResolvedNarrativeCombiId >= 0;
}

int OrchConductorAudioProcessor::getEffectiveCombiPresetId() const
{
    return isNarrativeScanDriving() ? lastResolvedNarrativeCombiId : combiPresetId;
}

bool OrchConductorAudioProcessor::isEffectiveCombiModeActive() const
{
    return isNarrativeScanDriving() || isCombiModeActive();
}

int OrchConductorAudioProcessor::getResolvedNarrativeCombiId() const
{
    return lastResolvedNarrativeCombiId;
}

int OrchConductorAudioProcessor::getResolvedNarrativeLanePointIndex() const
{
    return lastResolvedNarrativePointIndex;
}

int OrchConductorAudioProcessor::getNarrativeLaneCount() const
{
    return runtimePresetCatalog.getNarrativeLaneCount();
}

juce::String OrchConductorAudioProcessor::getNarrativeLaneLabel (int laneIndex) const
{
    if (laneIndex < 0 || laneIndex >= runtimePresetCatalog.getNarrativeLaneCount())
        return {};

    return runtimePresetCatalog.getNarrativeLaneLabel (laneIndex);
}

juce::String OrchConductorAudioProcessor::getNarrativeLanePointLabel (int laneIndex, int pointIndex) const
{
    if (laneIndex < 0 || laneIndex >= runtimePresetCatalog.getNarrativeLaneCount())
        return {};

    if (pointIndex < 0 || pointIndex >= runtimePresetCatalog.getNarrativeLanePointCount (laneIndex))
        return {};

    return runtimePresetCatalog.getNarrativeLanePointLabel (laneIndex, pointIndex);
}

juce::String OrchConductorAudioProcessor::getNarrativeLaneId (int laneIndex) const
{
    return runtimePresetCatalog.getNarrativeLaneId (laneIndex);
}

juce::String OrchConductorAudioProcessor::getNarrativeLaneDescription (int laneIndex) const
{
    return runtimePresetCatalog.getNarrativeLaneDescription (laneIndex);
}

int OrchConductorAudioProcessor::getNarrativeLanePointCount (int laneIndex) const
{
    return runtimePresetCatalog.getNarrativeLanePointCount (laneIndex);
}

double OrchConductorAudioProcessor::getNarrativeLanePointPosition (int laneIndex, int pointIndex) const
{
    return runtimePresetCatalog.getNarrativeLanePointPosition (laneIndex, pointIndex);
}

int OrchConductorAudioProcessor::getNarrativeLanePointCombiId (int laneIndex, int pointIndex) const
{
    return runtimePresetCatalog.getNarrativeLanePointCombiId (laneIndex, pointIndex);
}

int OrchConductorAudioProcessor::getNarrativeLanePointFieldIndex (int laneIndex, int pointIndex) const
{
    return runtimePresetCatalog.getNarrativeLanePointPitchFieldIndex (laneIndex, pointIndex);
}

int OrchConductorAudioProcessor::getNarrativeLanePointHarpValueAt (int laneIndex, int pointIndex) const
{
    return runtimePresetCatalog.getNarrativeLanePointHarpValue (laneIndex, pointIndex);
}

int OrchConductorAudioProcessor::getNarrativeLanePointPianoValueAt (int laneIndex, int pointIndex) const
{
    return runtimePresetCatalog.getNarrativeLanePointPianoValue (laneIndex, pointIndex);
}

int OrchConductorAudioProcessor::getNarrativeLanePointGateResponseModeAt (int laneIndex, int pointIndex) const
{
    return runtimePresetCatalog.getNarrativeLanePointGateResponseMode (laneIndex, pointIndex);
}

int OrchConductorAudioProcessor::getNarrativeLanePointGateResponseAmountAt (int laneIndex, int pointIndex) const
{
    return runtimePresetCatalog.getNarrativeLanePointGateResponseAmount (laneIndex, pointIndex);
}

namespace
{
    juce::String currentNarrativeLibraryJsonText (const juce::File& userFile)
    {
        if (userFile.existsAsFile())
            return userFile.loadFileAsString();   // File I/O strips a BOM for us

        const auto embedded = orchconductor::getEmbeddedFactoryJson();

        if (! embedded.isValid())
            return {};

        auto text = juce::String::fromUTF8 (embedded.data, embedded.size);

        if (text.startsWithChar (static_cast<juce::juce_wchar> (0xfeff)))
            text = text.substring (1);            // strip the UTF-8 BOM - juce::JSON::parse chokes on it

        return text;
    }
}

bool OrchConductorAudioProcessor::saveNarrativeLane (const juce::String& laneId,
                                                    const juce::String& name,
                                                    const juce::String& description,
                                                    const std::vector<NarrativeLanePointEdit>& pointsIn)
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    const auto libFile = getNarrativeLibraryFile();

    auto parsed = juce::JSON::parse (currentNarrativeLibraryJsonText (libFile));
    auto* root = parsed.getDynamicObject();

    if (root == nullptr)
        return false;

    if (! root->hasProperty ("narrativeLanes") || ! root->getProperty ("narrativeLanes").isArray())
        root->setProperty ("narrativeLanes", juce::Array<juce::var> {});

    auto* lanes = root->getProperty ("narrativeLanes").getArray();

    if (lanes == nullptr)
        return false;

    // Sort by position, drop points that would collide (must be strictly
    // ascending for the lane to validate).
    auto points = pointsIn;
    std::sort (points.begin(), points.end(),
               [] (const auto& a, const auto& b) { return a.position < b.position; });

    juce::Array<juce::var> pointArray;
    double lastPos = -1.0;

    for (auto p : points)
    {
        p.position = juce::jlimit (0.0, 1.0, p.position);

        if (p.position <= lastPos)
            p.position = juce::jmin (1.0, lastPos + 0.001);

        lastPos = p.position;

        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty ("position", p.position);
        o->setProperty ("combiId", p.combiId);

        if (p.pitchFieldIndex >= 0)   o->setProperty ("pitchFieldIndex", p.pitchFieldIndex);
        if (p.harpValue >= 0)         o->setProperty ("harpValue", p.harpValue);
        if (p.pianoValue >= 0)        o->setProperty ("pianoValue", p.pianoValue);
        if (p.gateResponseMode >= 0)  o->setProperty ("gateResponseMode", p.gateResponseMode);
        if (p.gateResponseAmount >= 0) o->setProperty ("gateResponseAmount", p.gateResponseAmount);

        pointArray.add (juce::var (o.get()));
    }

    juce::DynamicObject::Ptr lane = new juce::DynamicObject();
    lane->setProperty ("id", laneId);
    lane->setProperty ("name", name.isNotEmpty() ? name : laneId);
    lane->setProperty ("description", description);
    lane->setProperty ("points", pointArray);

    bool replaced = false;

    for (int i = 0; i < lanes->size(); ++i)
        if (auto* lo = (*lanes)[i].getDynamicObject();
            lo != nullptr && lo->getProperty ("id").toString() == laneId)
        {
            lanes->set (i, juce::var (lane.get()));
            replaced = true;
            break;
        }

    if (! replaced)
        lanes->add (juce::var (lane.get()));

    libFile.getParentDirectory().createDirectory();

    if (! libFile.replaceWithText (juce::JSON::toString (parsed, true)))
        return false;

    return importNarrativeLibraryFromFile (libFile);
#else
    juce::ignoreUnused (laneId, name, description, pointsIn);
    return false;
#endif
}

bool OrchConductorAudioProcessor::deleteNarrativeLane (const juce::String& laneId)
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    const auto libFile = getNarrativeLibraryFile();

    auto parsed = juce::JSON::parse (currentNarrativeLibraryJsonText (libFile));
    auto* root = parsed.getDynamicObject();

    if (root == nullptr || ! root->getProperty ("narrativeLanes").isArray())
        return false;

    auto* lanes = root->getProperty ("narrativeLanes").getArray();

    if (lanes == nullptr)
        return false;

    bool removed = false;

    for (int i = lanes->size(); --i >= 0;)
        if (auto* lo = (*lanes)[i].getDynamicObject();
            lo != nullptr && lo->getProperty ("id").toString() == laneId)
        {
            lanes->remove (i);
            removed = true;
        }

    if (! removed)
        return false;

    libFile.getParentDirectory().createDirectory();

    if (! libFile.replaceWithText (juce::JSON::toString (parsed, true)))
        return false;

    return importNarrativeLibraryFromFile (libFile);
#else
    juce::ignoreUnused (laneId);
    return false;
#endif
}

juce::StringArray OrchConductorAudioProcessor::getNarrativeArcShapeNames()
{
    return { "Organic Build", "Arch (Rise & Fall)", "Long Fade / Dissolution",
             "Terraced Blocks", "Surging Waves", "Heroic Journey",
             "Suspense -> Release", "Mosaic / Episodic",
             "Catastrophe / Collapse", "Pastoral Plateau" };
}

namespace
{
    struct ArcSample { float energy; float tension; };

    ArcSample sampleArc (OrchConductorAudioProcessor::NarrativeArcShape shape, float t)
    {
        using Shape = OrchConductorAudioProcessor::NarrativeArcShape;
        const float pi = juce::MathConstants<float>::pi;
        auto clamp01 = [] (float x) { return juce::jlimit (0.0f, 1.0f, x); };

        switch (shape)
        {
            case Shape::organicBuild:
                return { clamp01 (std::pow (t, 1.6f)), clamp01 (0.25f + 0.3f * t) };

            case Shape::archRiseFall:
                return { clamp01 (std::sin (t * pi)), clamp01 (0.3f + 0.25f * std::sin (t * pi)) };

            case Shape::longFade:
                return { clamp01 (std::pow (1.0f - t, 1.3f)), clamp01 (0.4f * (1.0f - t) + 0.15f) };

            case Shape::terracedBlocks:
            {
                const int step = juce::jlimit (0, 3, (int) (t * 3.999f));
                const float levels[] = { 0.2f, 0.5f, 0.35f, 0.9f };
                return { levels[step], 0.35f + 0.1f * step };
            }

            case Shape::surgingWaves:
            {
                const float base = 0.12f + 0.5f * t;
                const float swell = 0.32f * (0.5f - 0.5f * std::cos (t * 6.0f * pi));
                return { clamp01 (base + swell), clamp01 (0.3f + 0.2f * swell) };
            }

            case Shape::heroicJourney:
            {
                float e, tn;
                if (t < 0.28f)        { e = 0.42f + 0.06f * std::sin (t * 12.0f); tn = 0.35f; }
                else if (t < 0.52f)   { const float u = (t - 0.28f) / 0.24f; e = 0.4f - 0.24f * u; tn = 0.55f + 0.35f * u; }
                else                  { const float u = (t - 0.52f) / 0.48f; e = 0.18f + 0.82f * std::pow (u, 1.2f); tn = 0.6f - 0.35f * u; }
                return { clamp01 (e), clamp01 (tn) };
            }

            case Shape::suspenseRelease:
            {
                float e, tn;
                if (t < 0.72f)        { e = 0.16f + 0.14f * t; tn = 0.82f; }
                else if (t < 0.84f)   { const float u = (t - 0.72f) / 0.12f; e = 0.24f + 0.7f * u; tn = 0.82f - 0.5f * u; }
                else                  { const float u = (t - 0.84f) / 0.16f; e = 0.94f - 0.5f * u; tn = 0.32f; }
                return { clamp01 (e), clamp01 (tn) };
            }

            case Shape::mosaicEpisodic:
                return { clamp01 (0.5f + 0.42f * std::sin (t * 7.3f + 1.1f)),
                         clamp01 (0.5f + 0.4f * std::sin (t * 5.1f + 3.7f)) };

            case Shape::catastropheCollapse:
            {
                float e, tn;
                if (t < 0.56f)        { e = 0.82f * std::pow (t / 0.56f, 1.3f); tn = 0.4f + 0.4f * (t / 0.56f); }
                else if (t < 0.63f)   { const float u = (t - 0.56f) / 0.07f; e = 0.82f - 0.74f * u; tn = 0.95f - 0.3f * u; }
                else                  { const float u = (t - 0.63f) / 0.37f; e = 0.08f + 0.4f * u; tn = 0.5f - 0.2f * u; }
                return { clamp01 (e), clamp01 (tn) };
            }

            case Shape::pastoralPlateau:
            {
                float e;
                if (t < 0.22f)        e = (t / 0.22f) * 0.5f;
                else if (t < 0.85f)   e = 0.5f + 0.06f * std::sin (t * 8.0f * pi);
                else                  e = 0.5f + ((t - 0.85f) / 0.15f) * 0.22f;
                return { clamp01 (e), 0.2f };
            }
        }

        return { clamp01 (t), 0.35f };
    }

    // Approximate character of each factory combi (factoryId 0..28). User
    // combis fall back to an energy estimate from their explicit CC payload.
    struct CombiCharacter { float energy; float tension; float brightness; };

    CombiCharacter factoryCombiCharacter (int factoryId)
    {
        static const CombiCharacter table[] =
        {
            { 0.00f, 0.10f, 0.50f }, // 0  manual.sections (unused as a stop)
            { 0.00f, 0.10f, 0.50f }, // 1  all_off
            { 1.00f, 0.50f, 0.60f }, // 2  full_orchestra
            { 0.85f, 0.40f, 0.60f }, // 3  full_orchestra_no_percussion
            { 0.35f, 0.30f, 0.55f }, // 4  chamber_orchestra
            { 0.55f, 0.35f, 0.50f }, // 5  full_strings
            { 0.50f, 0.30f, 0.72f }, // 6  full_woodwinds
            { 0.80f, 0.55f, 0.55f }, // 7  full_brass
            { 0.70f, 0.40f, 0.62f }, // 8  full_winds
            { 0.65f, 0.35f, 0.85f }, // 9  high_orchestra
            { 0.62f, 0.50f, 0.20f }, // 10 low_orchestra
            { 0.55f, 0.35f, 0.50f }, // 11 middle_orchestra
            { 0.50f, 0.28f, 0.52f }, // 12 romantic.warm_strings_horns
            { 0.35f, 0.30f, 0.55f }, // 13 romantic.oboe_strings
            { 0.35f, 0.25f, 0.80f }, // 14 romantic.flute_violins
            { 0.42f, 0.35f, 0.22f }, // 15 romantic.bassoon_celli
            { 0.55f, 0.30f, 0.45f }, // 16 romantic.horn_choir_strings
            { 0.95f, 0.55f, 0.55f }, // 17 cinematic.heroic_brass_strings
            { 0.55f, 0.80f, 0.15f }, // 18 cinematic.dark_trailer_bed
            { 0.42f, 0.35f, 0.90f }, // 19 cinematic.high_winds_shimmer
            { 0.62f, 0.78f, 0.15f }, // 20 cinematic.epic_low_pulse
            { 0.35f, 0.70f, 0.22f }, // 21 herrmann.low_reeds
            { 0.60f, 0.85f, 0.40f }, // 22 herrmann.horn_knives
            { 0.55f, 0.95f, 0.40f }, // 23 herrmann.psycho_strings
            { 0.30f, 0.80f, 0.45f }, // 24 herrmann.suspense_winds
            { 0.32f, 0.62f, 0.60f }, // 25 modernist.pointillist_winds
            { 0.32f, 0.72f, 0.50f }, // 26 modernist.sparse_extremes
            { 0.35f, 0.25f, 0.95f }, // 27 shimmer.silver_shimmer
            { 0.20f, 0.40f, 0.40f }, // 28 solo.english_horn_lament
        };

        if (factoryId >= 0 && factoryId < (int) std::size (table))
            return table[factoryId];

        return { 0.5f, 0.4f, 0.5f };
    }
}

std::vector<OrchConductorAudioProcessor::NarrativeLanePointEdit>
    OrchConductorAudioProcessor::generateNarrativeLane (NarrativeArcShape shape,
                                                        int pointCount,
                                                        float restlessness,
                                                        juce::int64 seed) const
{
    juce::Random rng (seed);
    const int n = juce::jlimit (2, 16, pointCount);
    restlessness = juce::jlimit (0.0f, 1.0f, restlessness);

    // Candidate combis: every factory combi except manual.sections (0), plus
    // every user combi. Each carries an approximate character.
    struct Candidate { int combiId; CombiCharacter ch; };
    std::vector<Candidate> pool;

    for (int id = 1; id <= getMaxCombiPresetId(); ++id)
    {
        if (getCombiPresetLabel (id) == "Unknown Combi")
            continue;

        CombiCharacter ch;

        if (isUserCombiPresetId (id))
        {
            // Estimate energy from the combi's resolved CC payload.
            int total = 0, count = 0;

            for (int cc = 20; cc <= 62; ++cc)
            {
                if (cc == 49 || cc == 55) continue;
                total += getCombiResolvedCcValue (id, cc);
                ++count;
            }

            const float e = count > 0 ? juce::jlimit (0.0f, 1.0f, (float) total / (float) (count * 127)) * 1.4f : 0.4f;
            ch = { juce::jlimit (0.0f, 1.0f, e), 0.4f, 0.5f };
        }
        else
        {
            ch = factoryCombiCharacter (id);
        }

        pool.push_back ({ id, ch });
    }

    if (pool.empty())
        return {};

    auto pickForTarget = [&] (float e, float tn, float br, int avoidCombiId) -> int
    {
        std::vector<std::pair<float, int>> scored;   // (cost, combiId)

        for (const auto& c : pool)
        {
            if (c.combiId == avoidCombiId)
                continue;

            const float cost = 1.7f * std::abs (c.ch.energy - e)
                             + 1.1f * std::abs (c.ch.tension - tn)
                             + 0.6f * std::abs (c.ch.brightness - br);
            scored.push_back ({ cost, c.combiId });
        }

        if (scored.empty())
            return pool.front().combiId;

        std::sort (scored.begin(), scored.end(),
                   [] (const auto& a, const auto& b) { return a.first < b.first; });

        // Weighted pick from the top few - closer = more likely, but not always.
        const int shortlist = juce::jmin (4, (int) scored.size());
        const float roll = rng.nextFloat();
        const int idx = roll < 0.55f ? 0 : roll < 0.8f ? juce::jmin (1, shortlist - 1)
                       : roll < 0.93f ? juce::jmin (2, shortlist - 1)
                                      : juce::jmin (3, shortlist - 1);

        return scored[(size_t) idx].second;
    };

    std::vector<NarrativeLanePointEdit> lane;
    lane.reserve ((size_t) n);

    // OrchGate response bridge: the "mode" (per-instance seed) steps only when
    // the orchestration actually moves, so a held / reprised combi keeps the
    // same articulation attitude (coherence). The "amount" tracks the arc's
    // tension - unsettled passages let each OrchGate's invert / threshold /
    // participation wander further from its literal setting.
    int currentMode = 1 + (int) (((juce::uint64) seed) % 127u);

    for (int i = 0; i < n; ++i)
    {
        const float t = n > 1 ? (float) i / (float) (n - 1) : 0.0f;
        const auto s = sampleArc (shape, t);
        const float brightnessTarget = juce::jlimit (0.0f, 1.0f, 0.3f + 0.45f * t + 0.15f * s.energy);

        NarrativeLanePointEdit pt;
        pt.position = t;

        const int prevCombi = lane.empty() ? -1 : lane.back().combiId;

        // Coherence: hold or reprise rather than always moving on.
        const float holdProb    = (1.0f - restlessness) * 0.45f;
        const float repriseProb = (1.0f - restlessness) * 0.22f;
        const float roll = rng.nextFloat();

        if (i > 0 && i < n - 1 && prevCombi >= 0 && roll < holdProb)
        {
            pt.combiId = prevCombi;                       // held gesture
        }
        else if (i > 3 && i < n - 1 && roll < holdProb + repriseProb)
        {
            // reprise an earlier stop whose energy is near this target
            std::vector<int> candidates;

            for (int j = 0; j < (int) lane.size() - 1; ++j)
            {
                const auto lc = factoryCombiCharacter (lane[(size_t) j].combiId);
                if (std::abs (lc.energy - s.energy) < 0.22f)
                    candidates.push_back (lane[(size_t) j].combiId);
            }

            pt.combiId = candidates.empty()
                ? pickForTarget (s.energy, s.tension, brightnessTarget, prevCombi)
                : candidates[(size_t) rng.nextInt ((int) candidates.size())];
        }
        else
        {
            pt.combiId = pickForTarget (s.energy, s.tension, brightnessTarget,
                                        i == 0 ? -1 : prevCombi);
        }

        // A descending harmonic-field walk: complex early, simpler toward the
        // climax (mirrors the built-in organic_build lane's spirit).
        pt.pitchFieldIndex = juce::jlimit (0, maxPitchFieldIndex,
                                           juce::roundToInt ((1.0f - s.energy) * 10.0f));

        // Sprinkle harp/piano at the high points.
        if (s.energy > 0.72f && rng.nextFloat() < 0.45f)
            pt.harpValue = juce::jlimit (60, 127, juce::roundToInt (s.energy * 127.0f));

        if (s.energy > 0.85f && rng.nextFloat() < 0.35f)
            pt.pianoValue = juce::jlimit (70, 127, juce::roundToInt (s.energy * 127.0f));

        // Advance the response "chapter" only on a genuine orchestration move.
        if (i > 0 && pt.combiId != prevCombi)
            currentMode = 1 + ((currentMode + 17 + rng.nextInt (40)) % 127);

        pt.gateResponseMode = currentMode;
        pt.gateResponseAmount = juce::jlimit (0, 127,
            juce::roundToInt ((0.12f + 0.70f * s.tension + 0.18f * s.energy) * 127.0f));

        lane.push_back (pt);
    }

    return lane;
}

juce::String OrchConductorAudioProcessor::getCombiPresetName() const
{
    return getCombiPresetLabel (combiPresetId);
}

juce::String OrchConductorAudioProcessor::getCombiPresetLabel (int presetId) const
{
    if (presetId < minCombiPresetId || presetId > getMaxCombiPresetId())
        return "Unknown Combi";

    if (isUserCombiPresetId (presetId))
    {
        const auto it = userCombiPresets.find (presetId);

        if (it != userCombiPresets.end())
            return it->second.name;

        return "User Combi " + juce::String (presetId);
    }

    if (runtimePresetCatalogAuthorityActive)
    {
        const auto label = runtimePresetCatalog.getCombiPresetLabel (presetId);

        if (label.isNotEmpty())
            return label;
    }

    switch (static_cast<CombiPreset> (presetId))
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

bool OrchConductorAudioProcessor::getCombiPresetNarrativeMetadata (
    int presetId,
    orchconductor::NarrativeMetadata& metadata) const
{
    if (isUserCombiPresetId (presetId))
    {
        const auto it = userCombiPresets.find (presetId);

        if (it == userCombiPresets.end())
            return false;

        metadata = it->second.metadata;
        return true;
    }

    if (runtimePresetCatalogAuthorityActive)
        return runtimePresetCatalog.getCombiPresetNarrativeMetadata (presetId, metadata);

    return false;
}

juce::String OrchConductorAudioProcessor::getSectionPresetLabel (Section section, int presetId) const
{
    if (presetId < minSectionPresetId || presetId > getMaxSectionPresetId (section))
        return "Unknown";

    if (runtimePresetCatalogAuthorityActive)
    {
        const auto sectionId = getRuntimeCatalogSectionId (section);

        const auto label = sectionId.isNotEmpty()
            ? runtimePresetCatalog.getSectionPresetLabel (sectionId, presetId)
            : juce::String {};

        if (label.isNotEmpty())
            return label;
    }

    static const char* const woodwinds[] =
    {
        "All Off", "Piccolo Only", "Flutes", "Flute 1 Only", "Flute 2 Only",
        "Oboes", "Oboe 1 Only", "Oboe 2 Only", "English Horn Only",
        "Clarinets", "Clarinet 1 Only", "Clarinet 2 Only", "Bass Clarinet Only",
        "Bassoons", "Bassoon 1 Only", "Bassoon 2 Only", "Contrabassoon Only",
        "High Woodwinds", "Low Woodwinds", "Full Woodwinds",
        "Chamber Woodwinds",
        "High Orchestra Woodwinds",
        "Middle Reeds",
        "Extreme Low Woodwinds",
        "High Winds Shimmer",
        "Suspense Winds",
        "Pointillist Winds",
        "Sparse Extreme Woodwinds"
    };

    static const char* const brass[] =
    {
        "All Off", "Horns", "Horn 1 Only", "Horn 2 Only", "Horn 3 Only", "Horn 4 Only",
        "Trumpets", "Trumpet 1 Only", "Trumpet 2 Only", "Trumpet 3 Only",
        "Trombones", "Trombone 1 Only", "Trombone 2 Only", "Bass Trombone Only",
        "Tuba Only", "Low Brass", "Full Brass",
        "Dark Low Brass",
        "Sparse Brass Extremes"
    };

    static const char* const percussion[] =
    {
        "All Off", "Timpani Only", "Glockenspiel Only", "Xylophone Only", "Marimba Only",
        "Vibraphone Only", "Tubular Bells Only", "Mallets", "Full Melodic Percussion",
        "High Orchestra Percussion",
        "Middle Orchestra Percussion",
        "Shimmer Percussion",
        "Bass Drum Only", "Snare Drum Only", "Cymbals Only", "Piatti Only",
        "Tam-Tam Only", "Tambourine Only", "Triangle Only",
        "Unpitched Percussion",
        "Full Percussion"
    };

    switch (section)
    {
        case Section::woodwinds:
            return juce::isPositiveAndBelow (presetId, static_cast<int> (std::size (woodwinds))) ? woodwinds[presetId] : "Unknown Woodwinds";

        case Section::brass:
            return juce::isPositiveAndBelow (presetId, static_cast<int> (std::size (brass))) ? brass[presetId] : "Unknown Brass";

        case Section::percussion:
            return juce::isPositiveAndBelow (presetId, static_cast<int> (std::size (percussion))) ? percussion[presetId] : "Unknown Percussion";

        case Section::strings:
            if (presetId == 14)
                return "Middle Strings";

            if (presetId == 15)
                return "Sparse String Extremes";

            switch (static_cast<Preset> (presetId))
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

            return "Unknown Strings";
    }

    return "Unknown";
}

int OrchConductorAudioProcessor::getMaxCombiPresetId() const
{
    if (userCombiPresets.empty())
        return maxFactoryCombiPresetId;

    return std::max (maxFactoryCombiPresetId, userCombiPresets.rbegin()->first);
}

bool OrchConductorAudioProcessor::isUserCombiPresetId (int presetId) const
{
    return presetId >= firstUserCombiPresetId && presetId <= maxCombiPresetParameterId;
}

int OrchConductorAudioProcessor::getNextAvailableUserCombiPresetId() const
{
    for (int presetId = firstUserCombiPresetId; presetId <= maxCombiPresetParameterId; ++presetId)
    {
        if (userCombiPresets.find (presetId) == userCombiPresets.end())
            return presetId;
    }

    return -1;
}

bool OrchConductorAudioProcessor::getSectionPresetIdsForCombiPreset (int presetId,
                                                                     int& woodwinds,
                                                                     int& brass,
                                                                     int& percussion,
                                                                     int& strings) const
{
    if (isUserCombiPresetId (presetId))
    {
        const auto it = userCombiPresets.find (presetId);

        if (it == userCombiPresets.end())
            return false;

        woodwinds = it->second.woodwindsPresetId;
        brass = it->second.brassPresetId;
        percussion = it->second.percussionPresetId;
        strings = it->second.stringsPresetId;

        return true;
    }

    if (presetId == static_cast<int> (CombiPreset::manualSections))
    {
        woodwinds = woodwindsPresetId;
        brass = brassPresetId;
        percussion = percussionPresetId;
        strings = stringsPresetId;

        return true;
    }

    woodwinds = minSectionPresetId;
    brass = minSectionPresetId;
    percussion = minSectionPresetId;
    strings = minSectionPresetId;

    const auto valuesMatchAsGate = [] (int a, int b) -> bool
    {
        return (a >= 64) == (b >= 64);
    };

    const auto findMatch = [this, presetId, valuesMatchAsGate] (Section section) -> int
    {
        const int maxPresetId = getMaxSectionPresetId (section);

        for (int sectionPresetId = minSectionPresetId; sectionPresetId <= maxPresetId; ++sectionPresetId)
        {
            bool matches = true;

            switch (section)
            {
                case Section::woodwinds:
                {
                    for (int i = 0; i < numWoodwindsRows; ++i)
                    {
                        const int cc = woodwindsCcNumbers[i];

                        const int sectionValue = getSectionPresetValueForCc (section, sectionPresetId, cc);
                        const int combiValue = getCombiPresetValueForCc (presetId, cc);

                        if (! valuesMatchAsGate (sectionValue, combiValue))
                        {
                            matches = false;
                            break;
                        }
                    }

                    break;
                }

                case Section::brass:
                {
                    for (int i = 0; i < numBrassRows; ++i)
                    {
                        const int cc = brassCcNumbers[i];

                        const int sectionValue = getSectionPresetValueForCc (section, sectionPresetId, cc);
                        const int combiValue = getCombiPresetValueForCc (presetId, cc);

                        if (! valuesMatchAsGate (sectionValue, combiValue))
                        {
                            matches = false;
                            break;
                        }
                    }

                    break;
                }

                case Section::percussion:
                {
                    for (int i = 0; i < numPercussionRows; ++i)
                    {
                        const int cc = percussionCcNumbers[i];

                        const int sectionValue = getSectionPresetValueForCc (section, sectionPresetId, cc);
                        const int combiValue = getCombiPresetValueForCc (presetId, cc);

                        if (! valuesMatchAsGate (sectionValue, combiValue))
                        {
                            matches = false;
                            break;
                        }
                    }

                    break;
                }

                case Section::strings:
                {
                    for (int i = 0; i < numRows; ++i)
                    {
                        const int cc = ccNumbers[i];

                        const int sectionValue = getSectionPresetValueForCc (section, sectionPresetId, cc);
                        const int combiValue = getCombiPresetValueForCc (presetId, cc);

                        if (! valuesMatchAsGate (sectionValue, combiValue))
                        {
                            matches = false;
                            break;
                        }
                    }

                    break;
                }
            }

            if (matches)
                return sectionPresetId;
        }

        return minSectionPresetId;
    };

    woodwinds = findMatch (Section::woodwinds);
    brass = findMatch (Section::brass);
    percussion = findMatch (Section::percussion);
    strings = findMatch (Section::strings);

    return true;
}

juce::String OrchConductorAudioProcessor::createUserCombiNameFromCurrentSections() const
{
    juce::StringArray parts;

    auto makeCleanPart = [] (juce::String label)
    {
        label = label.trim();

        const auto firstSpace = label.indexOfChar (' ');

        if (firstSpace > 0)
        {
            const auto prefix = label.substring (0, firstSpace);
            bool prefixIsNumeric = prefix.isNotEmpty();

            for (auto c : prefix)
            {
                if (! juce::CharacterFunctions::isDigit (c))
                {
                    prefixIsNumeric = false;
                    break;
                }
            }

            if (prefixIsNumeric)
                label = label.substring (firstSpace + 1).trim();
        }

        if (label.endsWithIgnoreCase (" Only"))
            label = label.dropLastCharacters (5).trim();

        return label;
    };

    auto addPart = [&parts, &makeCleanPart] (int presetId, const juce::String& label)
    {
        if (presetId == 0)
            return;

        auto clean = makeCleanPart (label);

        if (clean.isNotEmpty())
            parts.add (clean);
    };

    addPart (getSectionPresetId (Section::woodwinds),
             getSectionPresetLabel (Section::woodwinds, getSectionPresetId (Section::woodwinds)));

    addPart (getSectionPresetId (Section::brass),
             getSectionPresetLabel (Section::brass, getSectionPresetId (Section::brass)));

    addPart (getSectionPresetId (Section::percussion),
             getSectionPresetLabel (Section::percussion, getSectionPresetId (Section::percussion)));

    addPart (getSectionPresetId (Section::strings),
             getSectionPresetLabel (Section::strings, getSectionPresetId (Section::strings)));

    if (parts.isEmpty())
        return "All Off";

    return parts.joinIntoString ("+");
}
int OrchConductorAudioProcessor::createUserCombiPresetFromCurrentSections (const juce::String& name)
{
    const auto presetId = getNextAvailableUserCombiPresetId();

    if (presetId < 0)
        return -1;

    UserCombiPreset preset;
    preset.name = name.isNotEmpty() ? name : "User Combi " + juce::String (presetId);
    preset.woodwindsPresetId = getSectionPresetId (Section::woodwinds);
    preset.brassPresetId = getSectionPresetId (Section::brass);
    preset.percussionPresetId = getSectionPresetId (Section::percussion);
    preset.stringsPresetId = getSectionPresetId (Section::strings);
    preset.harpValue = manualHarpValue;
    preset.pianoValue = manualPianoValue;

    userCombiPresets[presetId] = preset;

    saveUserCombiPresetsToUserLibrary();

    return presetId;
}

bool OrchConductorAudioProcessor::deleteUserCombiPreset (int presetId)
{
    if (! isUserCombiPresetId (presetId))
        return false;

    const auto erased = userCombiPresets.erase (presetId);

    if (erased == 0)
        return false;

    if (getCombiPresetId() == presetId)
        setCombiPresetIdFromUI (minCombiPresetId);

    saveUserCombiPresetsToUserLibrary();

    return true;
}

juce::File OrchConductorAudioProcessor::getUserCombiLibraryFile() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("OrchConductor");

    return dir.getChildFile ("UserCombiPresets.json");
}

bool OrchConductorAudioProcessor::importUserCombiPresetsFromJson (const juce::String& jsonText)
{
    auto parsed = juce::JSON::parse (jsonText);

    if (! parsed.isObject())
        return false;

    auto* root = parsed.getDynamicObject();

    if (root == nullptr)
        return false;

    const auto schema = root->getProperty ("schema").toString();

    if (schema.isNotEmpty() && schema != "orch_conductor_user_combi_presets")
        return false;

    auto combiPresetsVar = root->getProperty ("combiPresets");

    if (! combiPresetsVar.isArray())
        return false;

    auto* combiPresetsArray = combiPresetsVar.getArray();

    if (combiPresetsArray == nullptr)
        return false;

    decltype (userCombiPresets) importedPresets;

    int nextId = firstUserCombiPresetId;

    for (const auto& item : *combiPresetsArray)
    {
        if (! item.isObject())
            continue;

        auto* obj = item.getDynamicObject();

        if (obj == nullptr)
            continue;

        const auto name = obj->getProperty ("name").toString().trim();

        if (name.isEmpty())
            continue;

        auto sectionsVar = obj->getProperty ("sections");

        if (! sectionsVar.isObject())
            continue;

        auto* sections = sectionsVar.getDynamicObject();

        if (sections == nullptr)
            continue;

        UserCombiPreset preset;
        preset.name = name;
        preset.woodwindsPresetId = static_cast<int> (sections->getProperty ("woodwinds"));
        preset.brassPresetId = static_cast<int> (sections->getProperty ("brass"));
        preset.percussionPresetId = static_cast<int> (sections->getProperty ("percussion"));
        preset.stringsPresetId = static_cast<int> (sections->getProperty ("strings"));

        preset.harpValue = obj->hasProperty ("harpValue")
                                ? static_cast<int> (obj->getProperty ("harpValue")) : -1;
        preset.pianoValue = obj->hasProperty ("pianoValue")
                                ? static_cast<int> (obj->getProperty ("pianoValue")) : -1;

        // Arbitrary explicit CC overrides - same {cc, value} shape as the
        // factory library schema's combiPreset.values, so this vocabulary
        // is consistent across both JSON formats in this repo.
        auto explicitValuesVar = obj->getProperty ("values");

        if (explicitValuesVar.isArray())
        {
            for (const auto& valueItem : *explicitValuesVar.getArray())
            {
                if (! valueItem.isObject())
                    continue;

                auto* valueObj = valueItem.getDynamicObject();

                if (valueObj == nullptr || ! valueObj->hasProperty ("cc") || ! valueObj->hasProperty ("value"))
                    continue;

                orchconductor::PresetValue explicitValue;
                explicitValue.ccNumber = static_cast<int> (valueObj->getProperty ("cc"));
                explicitValue.value = static_cast<int> (valueObj->getProperty ("value"));

                if (explicitValue.isValid())
                    preset.explicitCcValues.push_back (explicitValue);
            }
        }

        readUserCombiNarrativeMetadata (*obj, preset.metadata);

        auto localId = static_cast<int> (obj->getProperty ("localId"));

        if (! isUserCombiPresetId (localId) || importedPresets.count (localId) != 0)
            localId = nextId;

        while (importedPresets.count (localId) != 0)
            ++localId;

        if (! isUserCombiPresetId (localId))
            continue;

        importedPresets[localId] = preset;
        nextId = juce::jmax (nextId, localId + 1);
    }

    if (importedPresets.empty() && ! combiPresetsArray->isEmpty())
        return false;

    userCombiPresets = std::move (importedPresets);
    return true;
}

void OrchConductorAudioProcessor::loadUserCombiPresetsFromUserLibrary()
{
    const auto file = getUserCombiLibraryFile();

    if (! file.existsAsFile())
        return;

    importUserCombiPresetsFromJson (file.loadFileAsString());
}

bool OrchConductorAudioProcessor::saveUserCombiPresetsToUserLibrary() const
{
    const auto file = getUserCombiLibraryFile();
    file.getParentDirectory().createDirectory();

    return writeUserCombiPresetsJsonToFile (file);
}

juce::String OrchConductorAudioProcessor::exportUserCombiPresetsToJson() const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("schema", "orch_conductor_user_combi_presets");
    root->setProperty ("version", 1);

    juce::Array<juce::var> combiPresets;

    for (const auto& [id, preset] : userCombiPresets)
    {
        juce::DynamicObject::Ptr presetObject = new juce::DynamicObject();
        presetObject->setProperty ("localId", id);
        presetObject->setProperty ("name", preset.name);

        juce::DynamicObject::Ptr sections = new juce::DynamicObject();
        sections->setProperty ("woodwinds", preset.woodwindsPresetId);
        sections->setProperty ("brass", preset.brassPresetId);
        sections->setProperty ("percussion", preset.percussionPresetId);
        sections->setProperty ("strings", preset.stringsPresetId);

        presetObject->setProperty ("sections", juce::var (sections.get()));

        // Harp/Piano: -1 (not overridden) is written as-is so re-import can
        // tell "unset" apart from an explicit 0.
        presetObject->setProperty ("harpValue", preset.harpValue);
        presetObject->setProperty ("pianoValue", preset.pianoValue);

        if (! preset.explicitCcValues.empty())
        {
            juce::Array<juce::var> values;

            for (const auto& explicitValue : preset.explicitCcValues)
            {
                juce::DynamicObject::Ptr valueObject = new juce::DynamicObject();
                valueObject->setProperty ("cc", explicitValue.ccNumber);
                valueObject->setProperty ("value", explicitValue.value);
                values.add (juce::var (valueObject.get()));
            }

            presetObject->setProperty ("values", values);
        }

        presetObject->setProperty (
            "metadata",
            juce::var (createNarrativeMetadataJsonObject (preset.metadata).get()));

        combiPresets.add (juce::var (presetObject.get()));
    }

    root->setProperty ("combiPresets", combiPresets);

    return juce::JSON::toString (juce::var (root.get()), true);
}

bool OrchConductorAudioProcessor::writeUserCombiPresetsJsonToFile (const juce::File& file) const
{
    return file.replaceWithText (exportUserCombiPresetsToJson());
}
int OrchConductorAudioProcessor::getMaxSectionPresetId (Section section) const
{
    switch (section)
    {
        case Section::woodwinds:  return maxWoodwindsPresetId;
        case Section::brass:      return maxBrassPresetId;
        case Section::percussion: return maxPercussionPresetId;
        case Section::strings:    return maxStringsPresetId;
    }

    return minSectionPresetId;
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
    juce::AudioParameterInt* parameter = nullptr;
    int* targetPresetId = nullptr;
    int minPresetId = minSectionPresetId;
    int maxPresetId = minSectionPresetId;

    switch (section)
    {
        case Section::woodwinds:
            parameter = woodwindsPresetParameter;
            targetPresetId = &woodwindsPresetId;
            maxPresetId = maxWoodwindsPresetId;
            break;

        case Section::brass:
            parameter = brassPresetParameter;
            targetPresetId = &brassPresetId;
            maxPresetId = maxBrassPresetId;
            break;

        case Section::percussion:
            parameter = percussionPresetParameter;
            targetPresetId = &percussionPresetId;
            maxPresetId = maxPercussionPresetId;
            break;

        case Section::strings:
            parameter = stringsPresetParameter;
            targetPresetId = &stringsPresetId;
            minPresetId = minStringsPresetId;
            maxPresetId = maxStringsPresetId;
            break;
    }

    if (targetPresetId == nullptr || presetId < minPresetId || presetId > maxPresetId)
        return;

    const bool changed = *targetPresetId != presetId;

    *targetPresetId = presetId;

    if (parameter != nullptr && parameter->get() != presetId)
        *parameter = presetId;

    if (changed && sendOnPresetChange)
        requestSendPreset();
}
void OrchConductorAudioProcessor::setCombiPresetIdFromUI (int presetId)
{
    if (combiPresetParameter != nullptr)
    {
        combiPresetParameter->beginChangeGesture();
        combiPresetParameter->setValueNotifyingHost (
            combiPresetParameter->convertTo0to1 (static_cast<float> (presetId)));
        combiPresetParameter->endChangeGesture();
    }

    setCombiPresetId (presetId);
}

void OrchConductorAudioProcessor::setSectionPresetIdFromUI (Section section, int presetId)
{
    juce::AudioParameterInt* parameter = nullptr;

    switch (section)
    {
        case Section::woodwinds:
            parameter = woodwindsPresetParameter;
            break;

        case Section::brass:
            parameter = brassPresetParameter;
            break;

        case Section::percussion:
            parameter = percussionPresetParameter;
            break;

        case Section::strings:
            parameter = stringsPresetParameter;
            break;
    }

    if (parameter != nullptr)
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (
            parameter->convertTo0to1 (static_cast<float> (presetId)));
        parameter->endChangeGesture();
    }

    setSectionPresetId (section, presetId);
}

void OrchConductorAudioProcessor::setManualHarpValue (int value)
{
    const int clamped = (value >= 0 && value <= 127) ? value : -1;

    if (clamped == manualHarpValue)
        return;

    manualHarpValue = clamped;

    if (sendOnPresetChange)
        requestSendPreset();
}

void OrchConductorAudioProcessor::setManualPianoValue (int value)
{
    const int clamped = (value >= 0 && value <= 127) ? value : -1;

    if (clamped == manualPianoValue)
        return;

    manualPianoValue = clamped;

    if (sendOnPresetChange)
        requestSendPreset();
}

int OrchConductorAudioProcessor::getManualHarpValue() const
{
    return manualHarpValue;
}

int OrchConductorAudioProcessor::getManualPianoValue() const
{
    return manualPianoValue;
}

void OrchConductorAudioProcessor::setSendOnPresetChangeFromUI (bool shouldSend)
{
    if (sendOnPresetChangeParameter != nullptr)
    {
        sendOnPresetChangeParameter->beginChangeGesture();
        sendOnPresetChangeParameter->setValueNotifyingHost (shouldSend ? 1.0f : 0.0f);
        sendOnPresetChangeParameter->endChangeGesture();
    }

    setSendOnPresetChange (shouldSend);
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
    explicitSendPresetRequested = true;
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

    if (sendOnPresetChangeParameter != nullptr
        && sendOnPresetChangeParameter->get() != shouldSend)
        *sendOnPresetChangeParameter = shouldSend;
}

bool OrchConductorAudioProcessor::getSendOnPresetChange() const
{
    return sendOnPresetChange;
}

OrchConductorAudioProcessor::InputPassthroughMode OrchConductorAudioProcessor::getInputPassthroughMode() const
{
    return inputPassthroughMode;
}

void OrchConductorAudioProcessor::setInputPassthroughMode (InputPassthroughMode mode)
{
    inputPassthroughMode = mode;

    if (inputPassthroughParameter != nullptr
        && inputPassthroughParameter->getIndex() != static_cast<int> (mode))
        *inputPassthroughParameter = static_cast<int> (mode);
}

void OrchConductorAudioProcessor::setInputPassthroughModeFromUI (InputPassthroughMode mode)
{
    if (inputPassthroughParameter != nullptr)
    {
        inputPassthroughParameter->beginChangeGesture();
        inputPassthroughParameter->setValueNotifyingHost (
            inputPassthroughParameter->convertTo0to1 (static_cast<float> (static_cast<int> (mode))));
        inputPassthroughParameter->endChangeGesture();
    }

    setInputPassthroughMode (mode);
}

juce::String OrchConductorAudioProcessor::getPresetName() const
{
    return getSectionPresetLabel (Section::strings, stringsPresetId);
}
int OrchConductorAudioProcessor::getNumOutputRows()
{
    return numRows;
}

OrchConductorAudioProcessor::OutputRow OrchConductorAudioProcessor::getOutputRow (int index) const
{
    if (index < 0 || index >= numRows)
        return makeOutputRow ("Invalid", 0, 0);

    const int cc = ccNumbers[index];
    int value = isEffectiveCombiModeActive()
        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), cc)
        : getPresetValueForIndex (index);

    if (isEffectiveCombiModeActive())
        tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), cc, value);
    else
        tryGetRuntimeSectionPresetValueForCc (Section::strings, stringsPresetId, cc, value);

    return makeOutputRow (instrumentNames[index], cc, value);
}

int OrchConductorAudioProcessor::getNumWoodwindsOutputRows()
{
    return numWoodwindsRows;
}

OrchConductorAudioProcessor::OutputRow OrchConductorAudioProcessor::getWoodwindsOutputRow (int index) const
{
    if (index < 0 || index >= numWoodwindsRows)
        return makeOutputRow ("Invalid", 0, 0);

    const int cc = woodwindsCcNumbers[index];
    int value = isEffectiveCombiModeActive()
        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), cc)
        : getWoodwindsPresetValueForIndex (index);

    if (isEffectiveCombiModeActive())
        tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), cc, value);
    else
        tryGetRuntimeSectionPresetValueForCc (Section::woodwinds, woodwindsPresetId, cc, value);

    return makeOutputRow (woodwindsInstrumentNames[index], cc, value);
}

int OrchConductorAudioProcessor::getNumBrassOutputRows()
{
    return numBrassRows;
}

OrchConductorAudioProcessor::OutputRow OrchConductorAudioProcessor::getBrassOutputRow (int index) const
{
    if (index < 0 || index >= numBrassRows)
        return makeOutputRow ("Invalid", 0, 0);

    const int cc = brassCcNumbers[index];
    int value = isEffectiveCombiModeActive()
        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), cc)
        : getBrassPresetValueForIndex (index);

    if (isEffectiveCombiModeActive())
        tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), cc, value);
    else
        tryGetRuntimeSectionPresetValueForCc (Section::brass, brassPresetId, cc, value);

    return makeOutputRow (brassInstrumentNames[index], cc, value);
}

int OrchConductorAudioProcessor::getNumPercussionOutputRows()
{
    return numPercussionRows;
}

OrchConductorAudioProcessor::OutputRow OrchConductorAudioProcessor::getPercussionOutputRow (int index) const
{
    if (index < 0 || index >= numPercussionRows)
        return makeOutputRow ("Invalid", 0, 0);

    const int cc = percussionCcNumbers[index];
    int value = isEffectiveCombiModeActive()
        ? getCombiPresetValueForCc (getEffectiveCombiPresetId(), cc)
        : getPercussionPresetValueForIndex (index);

    if (isEffectiveCombiModeActive())
        tryGetRuntimeCombiPresetValueForCc (getEffectiveCombiPresetId(), cc, value);
    else
        tryGetRuntimeSectionPresetValueForCc (Section::percussion, percussionPresetId, cc, value);

    return makeOutputRow (percussionInstrumentNames[index], cc, value);
}

int OrchConductorAudioProcessor::getPresetValueForIndex (int index) const
{
    if (index < 0 || index >= numRows)
        return 0;

    if (stringsPresetId == 14) // Middle Strings: CC51, CC52, CC53
        return (index == 1 || index == 2 || index == 3) ? 127 : 0;

    if (stringsPresetId == 15) // Sparse String Extremes: CC50, CC54
        return (index == 0 || index == 4) ? 127 : 0;

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
        case 20: // Chamber Woodwinds: CC21, CC23, CC26, CC29
            return (index == 1 || index == 3 || index == 6 || index == 9) ? 127 : 0;

        case 21: // High Orchestra Woodwinds: CC20, CC21, CC22, CC23, CC24
            return (index >= 0 && index <= 4) ? 127 : 0;

        case 22: // Middle Reeds: CC25, CC26, CC27
            return (index >= 5 && index <= 7) ? 127 : 0;

        case 23: // Extreme Low Woodwinds: CC28, CC31
            return (index == 8 || index == 11) ? 127 : 0;

        case 24: // High Winds Shimmer: CC20, CC21, CC22
            return (index >= 0 && index <= 2) ? 127 : 0;

        case 25: // Suspense Winds: CC23, CC25, CC26, CC28, CC29, CC30
            return (index == 3 || index == 5 || index == 6 || index == 8 || index == 9 || index == 10) ? 127 : 0;

        case 26: // Pointillist Winds: CC20, CC23, CC26, CC29
            return (index == 0 || index == 3 || index == 6 || index == 9) ? 127 : 0;

        case 27: // Sparse Extreme Woodwinds: CC20, CC31
            return (index == 0 || index == 11) ? 127 : 0;
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
        case 17: // Dark Low Brass: CC41, CC42
            return (index == 9 || index == 10) ? 127 : 0;

        case 18: // Sparse Brass Extremes: CC36, CC41
            return (index == 4 || index == 9) ? 127 : 0;
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
        case 9: // High Orchestra Percussion: CC44, CC45, CC47, CC48
            return (index == 1 || index == 2 || index == 4 || index == 5) ? 127 : 0;

        case 10: // Middle Orchestra Percussion: CC46, CC47
            return (index == 3 || index == 4) ? 127 : 0;

        case 11: // Shimmer Percussion: CC44, CC47, CC48
            return (index == 1 || index == 4 || index == 5) ? 127 : 0;

        case 12: return index == 6 ? 127 : 0;    // Bass Drum Only
        case 13: return index == 7 ? 127 : 0;    // Snare Drum Only
        case 14: return index == 8 ? 127 : 0;    // Cymbals Only
        case 15: return index == 9 ? 127 : 0;    // Piatti Only
        case 16: return index == 10 ? 127 : 0;   // Tam-Tam Only
        case 17: return index == 11 ? 127 : 0;   // Tambourine Only
        case 18: return index == 12 ? 127 : 0;   // Triangle Only

        case 19:                                 // Unpitched Percussion
            return (index >= 6 && index <= 12) ? 127 : 0;

        case 20: return 127;                     // Full Percussion (all 13 rows)
    }

    return 0;
}


int OrchConductorAudioProcessor::getStringsPresetValueForIndex (int presetId, int index) const
{
    if (index < 0 || index >= numRows)
        return 0;

    if (presetId == 14) // Middle Strings: CC51, CC52, CC53
        return (index == 1 || index == 2 || index == 3) ? 127 : 0;

    if (presetId == 15) // Sparse String Extremes: CC50, CC54
        return (index == 0 || index == 4) ? 127 : 0;

    switch (static_cast<Preset> (presetId))
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

int OrchConductorAudioProcessor::getWoodwindsPresetValueForIndex (int presetId, int index) const
{
    if (index < 0 || index >= numWoodwindsRows)
        return 0;

    switch (presetId)
    {
        case 0: return 0;                       // All Off

        case 1: return index == 0 ? 127 : 0;    // Piccolo Only

        case 2:                                 // Flutes
            return (index == 1 || index == 2) ? 127 : 0;
        case 3: return index == 1 ? 127 : 0;    // Flute 1 Only
        case 4: return index == 2 ? 127 : 0;    // Flute 2 Only

        case 5:                                 // Oboes
            return (index == 3 || index == 4) ? 127 : 0;
        case 6: return index == 3 ? 127 : 0;    // Oboe 1 Only
        case 7: return index == 4 ? 127 : 0;    // Oboe 2 Only
        case 8: return index == 5 ? 127 : 0;    // English Horn Only

        case 9:                                 // Clarinets
            return (index == 6 || index == 7) ? 127 : 0;
        case 10: return index == 6 ? 127 : 0;   // Clarinet 1 Only
        case 11: return index == 7 ? 127 : 0;   // Clarinet 2 Only
        case 12: return index == 8 ? 127 : 0;   // Bass Clarinet Only

        case 13:                                // Bassoons
            return (index == 9 || index == 10) ? 127 : 0;
        case 14: return index == 9 ? 127 : 0;   // Bassoon 1 Only
        case 15: return index == 10 ? 127 : 0;  // Bassoon 2 Only
        case 16: return index == 11 ? 127 : 0;  // Contrabassoon Only

        case 17:                                // High Woodwinds
            return (index >= 0 && index <= 7) ? 127 : 0;

        case 18:                                // Low Woodwinds
            return (index >= 8 && index <= 11) ? 127 : 0;

        case 19: return 127;                    // Full Woodwinds
        case 20: // Chamber Woodwinds: CC21, CC23, CC26, CC29
            return (index == 1 || index == 3 || index == 6 || index == 9) ? 127 : 0;

        case 21: // High Orchestra Woodwinds: CC20, CC21, CC22, CC23, CC24
            return (index >= 0 && index <= 4) ? 127 : 0;

        case 22: // Middle Reeds: CC25, CC26, CC27
            return (index >= 5 && index <= 7) ? 127 : 0;

        case 23: // Extreme Low Woodwinds: CC28, CC31
            return (index == 8 || index == 11) ? 127 : 0;

        case 24: // High Winds Shimmer: CC20, CC21, CC22
            return (index >= 0 && index <= 2) ? 127 : 0;

        case 25: // Suspense Winds: CC23, CC25, CC26, CC28, CC29, CC30
            return (index == 3 || index == 5 || index == 6 || index == 8 || index == 9 || index == 10) ? 127 : 0;

        case 26: // Pointillist Winds: CC20, CC23, CC26, CC29
            return (index == 0 || index == 3 || index == 6 || index == 9) ? 127 : 0;

        case 27: // Sparse Extreme Woodwinds: CC20, CC31
            return (index == 0 || index == 11) ? 127 : 0;
    }

    return 0;
}

int OrchConductorAudioProcessor::getBrassPresetValueForIndex (int presetId, int index) const
{
    if (index < 0 || index >= numBrassRows)
        return 0;

    switch (presetId)
    {
        case 0: return 0;                       // All Off

        case 1:                                 // Horns
            return (index >= 0 && index <= 3) ? 127 : 0;
        case 2: return index == 0 ? 127 : 0;    // Horn 1 Only
        case 3: return index == 1 ? 127 : 0;    // Horn 2 Only
        case 4: return index == 2 ? 127 : 0;    // Horn 3 Only
        case 5: return index == 3 ? 127 : 0;    // Horn 4 Only

        case 6:                                 // Trumpets
            return (index >= 4 && index <= 6) ? 127 : 0;
        case 7: return index == 4 ? 127 : 0;    // Trumpet 1 Only
        case 8: return index == 5 ? 127 : 0;    // Trumpet 2 Only
        case 9: return index == 6 ? 127 : 0;    // Trumpet 3 Only

        case 10:                                // Trombones
            return (index == 7 || index == 8) ? 127 : 0;
        case 11: return index == 7 ? 127 : 0;   // Trombone 1 Only
        case 12: return index == 8 ? 127 : 0;   // Trombone 2 Only
        case 13: return index == 9 ? 127 : 0;   // Bass Trombone Only

        case 14: return index == 10 ? 127 : 0;  // Tuba Only

        case 15:                                // Low Brass
            return (index >= 7 && index <= 10) ? 127 : 0;

        case 16: return 127;                    // Full Brass
        case 17: // Dark Low Brass: CC41, CC42
            return (index == 9 || index == 10) ? 127 : 0;

        case 18: // Sparse Brass Extremes: CC36, CC41
            return (index == 4 || index == 9) ? 127 : 0;
    }

    return 0;
}

int OrchConductorAudioProcessor::getPercussionPresetValueForIndex (int presetId, int index) const
{
    if (index < 0 || index >= numPercussionRows)
        return 0;

    switch (presetId)
    {
        case 0: return 0;                       // All Off
        case 1: return index == 0 ? 127 : 0;    // Timpani Only
        case 2: return index == 1 ? 127 : 0;    // Glockenspiel Only
        case 3: return index == 2 ? 127 : 0;    // Xylophone Only
        case 4: return index == 3 ? 127 : 0;    // Marimba Only
        case 5: return index == 4 ? 127 : 0;    // Vibraphone Only
        case 6: return index == 5 ? 127 : 0;    // Tubular Bells Only

        case 7:                                 // Mallets
            return (index >= 1 && index <= 5) ? 127 : 0;

        case 8: return 127;                     // Full Melodic Percussion
        case 9: // High Orchestra Percussion: CC44, CC45, CC47, CC48
            return (index == 1 || index == 2 || index == 4 || index == 5) ? 127 : 0;

        case 10: // Middle Orchestra Percussion: CC46, CC47
            return (index == 3 || index == 4) ? 127 : 0;

        case 11: // Shimmer Percussion: CC44, CC47, CC48
            return (index == 1 || index == 4 || index == 5) ? 127 : 0;

        case 12: return index == 6 ? 127 : 0;    // Bass Drum Only
        case 13: return index == 7 ? 127 : 0;    // Snare Drum Only
        case 14: return index == 8 ? 127 : 0;    // Cymbals Only
        case 15: return index == 9 ? 127 : 0;    // Piatti Only
        case 16: return index == 10 ? 127 : 0;   // Tam-Tam Only
        case 17: return index == 11 ? 127 : 0;   // Tambourine Only
        case 18: return index == 12 ? 127 : 0;   // Triangle Only

        case 19:                                 // Unpitched Percussion
            return (index >= 6 && index <= 12) ? 127 : 0;

        case 20: return 127;                     // Full Percussion (all 13 rows)
    }

    return 0;
}
int OrchConductorAudioProcessor::getSectionPresetValueForCc (Section section, int presetId, int ccNumber) const
{
    switch (section)
    {
        case Section::woodwinds:
            for (int i = 0; i < numWoodwindsRows; ++i)
                if (woodwindsCcNumbers[i] == ccNumber)
                    return getWoodwindsPresetValueForIndex (presetId, i);

            break;

        case Section::brass:
            for (int i = 0; i < numBrassRows; ++i)
                if (brassCcNumbers[i] == ccNumber)
                    return getBrassPresetValueForIndex (presetId, i);

            break;

        case Section::percussion:
            for (int i = 0; i < numPercussionRows; ++i)
                if (percussionCcNumbers[i] == ccNumber)
                    return getPercussionPresetValueForIndex (presetId, i);

            break;

        case Section::strings:
            for (int i = 0; i < numRows; ++i)
                if (ccNumbers[i] == ccNumber)
                    return getStringsPresetValueForIndex (presetId, i);

            break;
    }

    return 0;
}
bool OrchConductorAudioProcessor::isCombiSectionGateActive (int combiPresetIdToCheck, Section section) const
{
    const int* activeCcNumbers = nullptr;
    int numCcs = 0;

    switch (section)
    {
        case Section::woodwinds:
            activeCcNumbers = woodwindsCcNumbers;
            numCcs = numWoodwindsRows;
            break;

        case Section::brass:
            activeCcNumbers = brassCcNumbers;
            numCcs = numBrassRows;
            break;

        case Section::percussion:
            activeCcNumbers = percussionCcNumbers;
            numCcs = numPercussionRows;
            break;

        case Section::strings:
            activeCcNumbers = ccNumbers;
            numCcs = numRows;
            break;
    }

    for (int i = 0; i < numCcs; ++i)
    {
        const int cc = activeCcNumbers[i];
        const int value = getCombiPresetValueForCc (combiPresetIdToCheck, cc);

        if (value >= 64)
            return true;
    }

    return false;
}

juce::String OrchConductorAudioProcessor::getCombiSectionActiveCcDebugString (int combiPresetIdToCheck, Section section) const
{
    const int* activeCcNumbers = nullptr;
    int numCcs = 0;

    switch (section)
    {
        case Section::woodwinds:
            activeCcNumbers = woodwindsCcNumbers;
            numCcs = numWoodwindsRows;
            break;

        case Section::brass:
            activeCcNumbers = brassCcNumbers;
            numCcs = numBrassRows;
            break;

        case Section::percussion:
            activeCcNumbers = percussionCcNumbers;
            numCcs = numPercussionRows;
            break;

        case Section::strings:
            activeCcNumbers = ccNumbers;
            numCcs = numRows;
            break;
    }

    juce::String result;

    for (int i = 0; i < numCcs; ++i)
    {
        const int cc = activeCcNumbers[i];
        const int value = getCombiPresetValueForCc (combiPresetIdToCheck, cc);

        if (value >= 64)
        {
            if (result.isNotEmpty())
                result += ", ";

            result += "CC" + juce::String (cc);
        }
    }

    if (result.isEmpty())
        result = "(none)";

    return result;
}

void OrchConductorAudioProcessor::debugValidateFactoryCombiSectionCoverage() const
{
    DBG ("");
    DBG ("=== Factory Combi Section Coverage / Atom-Molecule Normalization ===");

    int unresolvedCount = 0;

    for (int presetId = 0; presetId <= maxFactoryCombiPresetId; ++presetId)
    {
        if (isUserCombiPresetId (presetId))
            continue;

        int woodwindsPresetId = 0;
        int brassPresetId = 0;
        int percussionPresetId = 0;
        int stringsPresetId = 0;

        const bool resolved = getSectionPresetIdsForCombiPreset (
            presetId,
            woodwindsPresetId,
            brassPresetId,
            percussionPresetId,
            stringsPresetId);

        const bool woodwindsActive = isCombiSectionGateActive (presetId, Section::woodwinds);
        const bool brassActive = isCombiSectionGateActive (presetId, Section::brass);
        const bool percussionActive = isCombiSectionGateActive (presetId, Section::percussion);
        const bool stringsActive = isCombiSectionGateActive (presetId, Section::strings);

        bool hasUnresolvedActiveSection = false;

        if (woodwindsActive && woodwindsPresetId == 0)
            hasUnresolvedActiveSection = true;

        if (brassActive && brassPresetId == 0)
            hasUnresolvedActiveSection = true;

        if (percussionActive && percussionPresetId == 0)
            hasUnresolvedActiveSection = true;

        if (stringsActive && stringsPresetId == 0)
            hasUnresolvedActiveSection = true;

        if (hasUnresolvedActiveSection || ! resolved)
        {
            ++unresolvedCount;

            DBG ("UNRESOLVED COMBI " << presetId << " " << getCombiPresetLabel (presetId));
            DBG ("  Resolved flag: " << (resolved ? "true" : "false"));
            DBG ("  Revealed atoms: WW " << woodwindsPresetId
                 << ", BR " << brassPresetId
                 << ", PC " << percussionPresetId
                 << ", ST " << stringsPresetId);

            if (woodwindsActive && woodwindsPresetId == 0)
                DBG ("  Missing Woodwinds atom: " << getCombiSectionActiveCcDebugString (presetId, Section::woodwinds));

            if (brassActive && brassPresetId == 0)
                DBG ("  Missing Brass atom: " << getCombiSectionActiveCcDebugString (presetId, Section::brass));

            if (percussionActive && percussionPresetId == 0)
                DBG ("  Missing Percussion atom: " << getCombiSectionActiveCcDebugString (presetId, Section::percussion));

            if (stringsActive && stringsPresetId == 0)
                DBG ("  Missing Strings atom: " << getCombiSectionActiveCcDebugString (presetId, Section::strings));
        }
    }

    DBG ("Factory combi unresolved molecule count: " << unresolvedCount);
    DBG ("=== End Factory Combi Section Coverage ===");
    DBG ("");
}

int OrchConductorAudioProcessor::getCombiPresetValueForCc (int ccNumber) const
{
    return getCombiPresetValueForCc (combiPresetId, ccNumber);
}
int OrchConductorAudioProcessor::getCombiPresetValueForCc (int presetId, int ccNumber) const
{
    if (isUserCombiPresetId (presetId))
    {
        const auto it = userCombiPresets.find (presetId);

        if (it == userCombiPresets.end())
            return 0;

        const auto& preset = it->second;

        // Arbitrary explicit CC overrides take precedence over everything
        // else, including Harp/Piano and the section composition below -
        // this is what lets a combi express something finer than "pick a
        // whole named section preset per family" (e.g. one string entering
        // at 40 rather than full value, for a graduated
        // accumulation/transition/fade-out).
        for (const auto& explicitValue : preset.explicitCcValues)
            if (explicitValue.ccNumber == ccNumber)
                return explicitValue.value;

        // Harp/Piano don't belong to any of the 4 section families composed
        // below, so they're resolved as an explicit per-combi override first.
        if (ccNumber == reservedHarpCcNumber)
            return preset.harpValue >= 0 ? preset.harpValue : 0;

        if (ccNumber == pianoCcNumber)
            return preset.pianoValue >= 0 ? preset.pianoValue : 0;

        auto value = getSectionPresetValueForCc (Section::woodwinds, preset.woodwindsPresetId, ccNumber);

        if (value != 0)
            return value;

        value = getSectionPresetValueForCc (Section::brass, preset.brassPresetId, ccNumber);

        if (value != 0)
            return value;

        value = getSectionPresetValueForCc (Section::percussion, preset.percussionPresetId, ccNumber);

        if (value != 0)
            return value;

        return getSectionPresetValueForCc (Section::strings, preset.stringsPresetId, ccNumber);
    }

    const auto combi = static_cast<CombiPreset> (presetId);

    switch (combi)
    {
        case CombiPreset::manualSections:
            return 0;

        case CombiPreset::utilityAllOff:
            return 0;

        case CombiPreset::utilityFullOrchestra:
            // Was missing the 7 unpitched-percussion CCs (56-62) entirely -
            // every factory combi predates that instrument family. Live-rig
            // bug 2026-09-10: "Full Orchestra" was silently sending zero
            // percussion for that whole family regardless of intent.
            return ((ccNumber >= 20 && ccNumber <= 48) || (ccNumber >= 50 && ccNumber <= 54)
                 || (ccNumber >= 56 && ccNumber <= 62)) ? 127 : 0;

        case CombiPreset::utilityFullOrchestraNoPercussion:
            // Deliberately excludes 43-48 and 56-62 both - the name says so.
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
            // Unpitched additions by register (2026-09-10, see
            // utilityFullOrchestra's note): Cymbals/Piatti/Triangle are the
            // high-register voices of the family.
            return (ccNumber == 20 || ccNumber == 21 || ccNumber == 22 || ccNumber == 23 || ccNumber == 24
                 || ccNumber == 36 || ccNumber == 37 || ccNumber == 38
                 || ccNumber == 44 || ccNumber == 45 || ccNumber == 47 || ccNumber == 48
                 || ccNumber == 50 || ccNumber == 51
                 || ccNumber == 58 || ccNumber == 59 || ccNumber == 62) ? 127 : 0;

        case CombiPreset::utilityLowOrchestra:
            // Bass Drum/Tam-Tam are the low-register voices of the family.
            return (ccNumber == 28 || ccNumber == 29 || ccNumber == 30 || ccNumber == 31
                 || ccNumber == 39 || ccNumber == 40 || ccNumber == 41 || ccNumber == 42
                 || ccNumber == 43 || ccNumber == 52 || ccNumber == 53 || ccNumber == 54
                 || ccNumber == 56 || ccNumber == 60) ? 127 : 0;

        case CombiPreset::utilityMiddleOrchestra:
            // Snare Drum/Tambourine are the middle-register voices of the family.
            return (ccNumber == 25 || ccNumber == 26 || ccNumber == 27
                 || ccNumber == 32 || ccNumber == 33 || ccNumber == 34 || ccNumber == 35
                 || ccNumber == 46 || ccNumber == 47
                 || ccNumber == 51 || ccNumber == 52 || ccNumber == 53
                 || ccNumber == 57 || ccNumber == 61) ? 127 : 0;

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
            // Heroic hits: Bass Drum + Cymbals/Piatti crashes + Tam-Tam weight.
            return ((ccNumber >= 32 && ccNumber <= 42) || (ccNumber >= 50 && ccNumber <= 54) || ccNumber == 43
                 || ccNumber == 56 || ccNumber == 58 || ccNumber == 59 || ccNumber == 60) ? 127 : 0;

        case CombiPreset::cinematicDarkTrailerBed:
            // Classic trailer-bed boom: Bass Drum + Tam-Tam.
            return (ccNumber == 28 || ccNumber == 31 || ccNumber == 41 || ccNumber == 42
                 || ccNumber == 43 || ccNumber == 53 || ccNumber == 54
                 || ccNumber == 56 || ccNumber == 60) ? 127 : 0;

        case CombiPreset::cinematicHighWindsShimmer:
            // Shimmer/swell voices: Cymbals + Triangle.
            return (ccNumber == 20 || ccNumber == 21 || ccNumber == 22
                 || ccNumber == 44 || ccNumber == 47 || ccNumber == 48
                 || ccNumber == 50 || ccNumber == 51
                 || ccNumber == 58 || ccNumber == 62) ? 127 : 0;

        case CombiPreset::cinematicEpicLowPulse:
            // Epic low pulse: Bass Drum + Tam-Tam.
            return (ccNumber == 28 || ccNumber == 31 || ccNumber == 39 || ccNumber == 40 || ccNumber == 41 || ccNumber == 42
                 || ccNumber == 43 || ccNumber == 53 || ccNumber == 54
                 || ccNumber == 56 || ccNumber == 60) ? 127 : 0;

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
            // "Extremes" already spans top-to-bottom register-wise - Triangle
            // (highest) and Tam-Tam (lowest, most extreme) fit the concept.
            return (ccNumber == 20 || ccNumber == 31 || ccNumber == 36 || ccNumber == 41 || ccNumber == 45 || ccNumber == 50 || ccNumber == 54
                 || ccNumber == 60 || ccNumber == 62) ? 127 : 0;

        case CombiPreset::shimmerSilverShimmer:
            // Shimmer voices: Cymbals + Triangle, matching cinematicHighWindsShimmer.
            return (ccNumber == 20 || ccNumber == 21 || ccNumber == 22
                 || ccNumber == 44 || ccNumber == 47 || ccNumber == 48
                 || ccNumber == 50 || ccNumber == 51
                 || ccNumber == 58 || ccNumber == 62) ? 127 : 0;

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


