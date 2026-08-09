#include "OrchConductorProcessor.h"
#include <cmath>
#include "OrchConductorEditor.h"
#include "OrchConductorRuntimePresetSource.h"
#include "OrchConductorRuntimePresetCatalog.h"

namespace
{

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

            bool sawReservedCc49 = false;

            for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex)
            {
                const auto value = catalog.getCombiPresetValue(presetId, valueIndex);

                if (! runtimeCatalogValueIsMidiSafe(value))
                    return false;

                if (value.ccNumber == 49)
                {
                    sawReservedCc49 = true;

                    if (value.value != 0)
                        return false;
                }
            }

            if (valueCount >= 35 && ! sawReservedCc49)
                return false;
        }

        return true;
    }

    bool verifyRuntimeCatalogCoverageAudit(const OrchConductorRuntimePresetCatalog& catalog)
    {
        return verifyRuntimeCatalogSectionPresetRange(catalog, "woodwinds", 0, 19)
            && verifyRuntimeCatalogSectionPresetRange(catalog, "brass", 0, 16)
            && verifyRuntimeCatalogSectionPresetRange(catalog, "percussion", 0, 8)
            && verifyRuntimeCatalogSectionPresetRange(catalog, "strings", 0, 12)
            && verifyRuntimeCatalogCombiPresetRange(catalog, 0, 28);
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
    addParameter (combiPresetParameter = new juce::AudioParameterInt (
        juce::ParameterID { "combiPreset", 1 },
        "Combi Preset",
        minCombiPresetId,
        maxCombiPresetId,
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
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    const auto runtimeJsonPresetProbe = orchconductor::RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled();

    runtimeJsonPresetProbeLoaded = runtimeJsonPresetProbe.wasLoaded();
    runtimeJsonPresetProbeRequiresFallback = runtimeJsonPresetProbe.requiresHardcodedFallback();
    runtimeJsonPresetProbeDiagnostic = runtimeJsonPresetProbe.diagnosticMessage;

    const auto runtimePresetCatalogAuthorityProbe =
        OrchConductorRuntimePresetCatalog::createFromRuntimeSource(runtimeJsonPresetProbe);

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
            verifyRuntimeCatalogPayloadEquivalenceSentinels(runtimePresetCatalogAuthorityProbe);

        runtimeCatalogPayloadEquivalenceProbeDiagnostic =
            runtimeCatalogPayloadEquivalenceProbePassed
                ? "Runtime catalog payload equivalence probe passed for broadened sentinel hardcoded presets."
                : "Runtime catalog payload equivalence probe failed for broadened sentinel hardcoded presets.";

        runtimeCatalogCoverageAuditRun = true;
        runtimeCatalogCoverageAuditBlockedByFallback = false;
        runtimeCatalogCoverageAuditPassed =
            verifyRuntimeCatalogCoverageAudit(runtimePresetCatalogAuthorityProbe);

        runtimeCatalogCoverageAuditDiagnostic =
            runtimeCatalogCoverageAuditPassed
                ? "Runtime catalog coverage audit passed for expected factory catalog shape."
                : "Runtime catalog coverage audit failed for expected factory catalog shape.";
#if ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL
        runtimeCatalogAuthorityTrialRun = true;
        runtimeCatalogAuthorityTrialBlocked = false;
        runtimeCatalogAuthorityTrialPass =
            verifyRuntimeCatalogAuthorityTrialSentinels(runtimePresetCatalogAuthorityProbe);

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

        if (runtimePresetCatalogAuthorityActive)
            runtimePresetCatalog = runtimePresetCatalogAuthorityProbe;

    }
    else
    {
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

    // Harp is reserved at CC49 and currently always inactive in the UI layer.

    for (int i = 0; i < getNumOutputRows(); ++i)
        total += getOutputRow (i).activePlayers;

    return total;
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
}
bool OrchConductorAudioProcessor::tryGetRuntimeSectionPresetValueForCc (Section section,
                                                                        int presetId,
                                                                        int ccNumber,
                                                                        int& value) const
{
    if (! runtimePresetCatalogAuthorityActive)
        return false;

    juce::String sectionId;

    switch (section)
    {
        case Section::woodwinds:
            sectionId = "woodwinds";
            break;

        case Section::brass:
            sectionId = "brass";
            break;

        case Section::percussion:
            sectionId = "percussion";
            break;

        case Section::strings:
            sectionId = "strings";
            break;
    }

    const int valueCount = runtimePresetCatalog.getSectionPresetValueCount (sectionId, presetId);

    for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex)
    {
        const auto runtimeValue = runtimePresetCatalog.getSectionPresetValue (sectionId, presetId, valueIndex);

        if (runtimeValue.isValid
            && runtimeValue.ccNumber == ccNumber
            && runtimeValue.value >= 0
            && runtimeValue.value <= 127)
        {
            value = runtimeValue.value;
            return true;
        }
    }

    return false;
}

bool OrchConductorAudioProcessor::tryGetRuntimeCombiPresetValueForCc (int presetId,
                                                                      int ccNumber,
                                                                      int& value) const
{
    if (! runtimePresetCatalogAuthorityActive)
        return false;

    const int valueCount = runtimePresetCatalog.getCombiPresetValueCount (presetId);

    for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex)
    {
        const auto runtimeValue = runtimePresetCatalog.getCombiPresetValue (presetId, valueIndex);

        if (runtimeValue.isValid
            && runtimeValue.ccNumber == ccNumber
            && runtimeValue.value >= 0
            && runtimeValue.value <= 127)
        {
            value = runtimeValue.value;
            return true;
        }
    }

    return false;
}
void OrchConductorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    syncAutomatedParameters();

    const bool shouldSendAllOff = consumeSendAllOffRequest();
    const bool shouldSendPreset = consumeSendPresetRequest();

    if (! shouldSendAllOff && ! shouldSendPreset)
        return;

    const bool useCombi = isCombiModeActive() && ! shouldSendAllOff;

    for (int i = 0; i < numWoodwindsRows; ++i)
    {
        const int cc = woodwindsCcNumbers[i];

        int value = shouldSendAllOff ? 0
                  : useCombi        ? getCombiPresetValueForCc (cc)
                                    : getWoodwindsPresetValueForIndex (i);

        if (! shouldSendAllOff)
        {
            if (useCombi)
                tryGetRuntimeCombiPresetValueForCc (combiPresetId, cc, value);

            if (! useCombi)
                tryGetRuntimeSectionPresetValueForCc (Section::woodwinds, woodwindsPresetId, cc, value);
        }

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    for (int i = 0; i < numBrassRows; ++i)
    {
        const int cc = brassCcNumbers[i];

        int value = shouldSendAllOff ? 0
                  : useCombi        ? getCombiPresetValueForCc (cc)
                                    : getBrassPresetValueForIndex (i);

        if (! shouldSendAllOff)
        {
            if (useCombi)
                tryGetRuntimeCombiPresetValueForCc (combiPresetId, cc, value);

            if (! useCombi)
                tryGetRuntimeSectionPresetValueForCc (Section::brass, brassPresetId, cc, value);
        }

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    for (int i = 0; i < numPercussionRows; ++i)
    {
        const int cc = percussionCcNumbers[i];

        int value = shouldSendAllOff ? 0
                  : useCombi        ? getCombiPresetValueForCc (cc)
                                    : getPercussionPresetValueForIndex (i);

        if (! shouldSendAllOff)
        {
            if (useCombi)
                tryGetRuntimeCombiPresetValueForCc (combiPresetId, cc, value);

            if (! useCombi)
                tryGetRuntimeSectionPresetValueForCc (Section::percussion, percussionPresetId, cc, value);
        }

        midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
    }

    int reservedCc49Value = 0;

    if (! shouldSendAllOff && useCombi)
        tryGetRuntimeCombiPresetValueForCc (combiPresetId, 49, reservedCc49Value);

    midiMessages.addEvent (juce::MidiMessage::controllerEvent (1, 49, reservedCc49Value), 0);

    for (int i = 0; i < numRows; ++i)
    {
        const int cc = ccNumbers[i];

        int value = shouldSendAllOff ? 0
                  : useCombi        ? getCombiPresetValueForCc (cc)
                                    : getPresetValueForIndex (i);

        if (! shouldSendAllOff)
        {
            if (useCombi)
                tryGetRuntimeCombiPresetValueForCc (combiPresetId, cc, value);

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
            setSendOnPresetChange (stream.readBool());

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
    if (presetId < minCombiPresetId || presetId > maxCombiPresetId)
        return;

    const bool changed = combiPresetId != presetId;

    combiPresetId = presetId;

    if (combiPresetParameter != nullptr && combiPresetParameter->get() != presetId)
        *combiPresetParameter = presetId;

    if (changed && sendOnPresetChange)
        requestSendPreset();
}
bool OrchConductorAudioProcessor::isCombiModeActive() const
{
    return combiPresetId != static_cast<int> (CombiPreset::manualSections);
}

juce::String OrchConductorAudioProcessor::getCombiPresetName() const
{
    return getCombiPresetLabel (combiPresetId);
}

juce::String OrchConductorAudioProcessor::getCombiPresetLabel (int presetId) const
{
    if (presetId < minCombiPresetId || presetId > maxCombiPresetId)
        return "Unknown Combi";

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

juce::String OrchConductorAudioProcessor::getSectionPresetLabel (Section section, int presetId) const
{
    if (presetId < minSectionPresetId || presetId > getMaxSectionPresetId (section))
        return "Unknown";

    if (runtimePresetCatalogAuthorityActive)
    {
        juce::String sectionId;

        switch (section)
        {
            case Section::woodwinds:  sectionId = "woodwinds"; break;
            case Section::brass:      sectionId = "brass"; break;
            case Section::percussion: sectionId = "percussion"; break;
            case Section::strings:    sectionId = "strings"; break;
        }

        const auto label = runtimePresetCatalog.getSectionPresetLabel (sectionId, presetId);

        if (label.isNotEmpty())
            return label;
    }

    static const char* const woodwinds[] =
    {
        "All Off", "Piccolo Only", "Flutes", "Flute 1 Only", "Flute 2 Only",
        "Oboes", "Oboe 1 Only", "Oboe 2 Only", "English Horn Only",
        "Clarinets", "Clarinet 1 Only", "Clarinet 2 Only", "Bass Clarinet Only",
        "Bassoons", "Bassoon 1 Only", "Bassoon 2 Only", "Contrabassoon Only",
        "High Woodwinds", "Low Woodwinds", "Full Woodwinds"
    };

    static const char* const brass[] =
    {
        "All Off", "Horns", "Horn 1 Only", "Horn 2 Only", "Horn 3 Only", "Horn 4 Only",
        "Trumpets", "Trumpet 1 Only", "Trumpet 2 Only", "Trumpet 3 Only",
        "Trombones", "Trombone 1 Only", "Trombone 2 Only", "Bass Trombone Only",
        "Tuba Only", "Low Brass", "Full Brass"
    };

    static const char* const percussion[] =
    {
        "All Off", "Timpani Only", "Glockenspiel Only", "Xylophone Only", "Marimba Only",
        "Vibraphone Only", "Tubular Bells Only", "Mallets", "Full Melodic Percussion"
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
    return maxCombiPresetId;
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
    int value = isCombiModeActive()
        ? getCombiPresetValueForCc (cc)
        : getPresetValueForIndex (index);

    if (isCombiModeActive())
        tryGetRuntimeCombiPresetValueForCc (combiPresetId, cc, value);
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
    int value = isCombiModeActive()
        ? getCombiPresetValueForCc (cc)
        : getWoodwindsPresetValueForIndex (index);

    if (isCombiModeActive())
        tryGetRuntimeCombiPresetValueForCc (combiPresetId, cc, value);
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
    int value = isCombiModeActive()
        ? getCombiPresetValueForCc (cc)
        : getBrassPresetValueForIndex (index);

    if (isCombiModeActive())
        tryGetRuntimeCombiPresetValueForCc (combiPresetId, cc, value);
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
    int value = isCombiModeActive()
        ? getCombiPresetValueForCc (cc)
        : getPercussionPresetValueForIndex (index);

    if (isCombiModeActive())
        tryGetRuntimeCombiPresetValueForCc (combiPresetId, cc, value);
    else
        tryGetRuntimeSectionPresetValueForCc (Section::percussion, percussionPresetId, cc, value);

    return makeOutputRow (percussionInstrumentNames[index], cc, value);
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

