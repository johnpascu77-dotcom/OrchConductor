#include <JuceHeader.h>

#include "../Source/OrchConductorProcessor.h"

#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace
{

constexpr int expectedMidiChannel = 1;
constexpr int expectedSendCcCount = 35;

int fail(const juce::String& message)
{
    std::cerr << "[FAIL] " << message << std::endl;
    return 1;
}

bool checkPass(bool condition, const juce::String& label)
{
    if (! condition)
    {
        std::cerr << "[FAIL] " << label << std::endl;
        return false;
    }

    std::cout << "[PASS] " << label << std::endl;
    return true;
}

bool checkEquals(int actual, int expected, const juce::String& label)
{
    if (actual != expected)
    {
        std::cerr << "[FAIL] " << label
                  << ": expected " << expected
                  << ", got " << actual
                  << std::endl;
        return false;
    }

    std::cout << "[PASS] " << label << ": " << actual << std::endl;
    return true;
}

struct CapturedMidi
{
    int eventCount = 0;
    std::map<int, int> ccValues;
    std::vector<int> channels;
};

CapturedMidi captureMidi(OrchConductorAudioProcessor& processor)
{
    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;

    buffer.clear();
    processor.processBlock(buffer, midi);

    CapturedMidi captured;
    captured.eventCount = midi.getNumEvents();

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isController())
        {
            captured.channels.push_back(message.getChannel());
            captured.ccValues[message.getControllerNumber()] = message.getControllerValue();
        }
    }

    return captured;
}

bool verifyNoMidiWithoutRequest()
{
    OrchConductorAudioProcessor processor;

    const auto captured = captureMidi(processor);

    bool ok = true;

    ok = checkEquals(captured.eventCount, 0, "no MIDI event count without pending request") && ok;
    ok = checkEquals(static_cast<int>(captured.ccValues.size()), 0, "no MIDI CC values without pending request") && ok;

    return ok;
}

bool verifyStandardSendShape(const CapturedMidi& captured, const juce::String& context)
{
    bool ok = true;

    ok = checkEquals(captured.eventCount, expectedSendCcCount, context + " event count") && ok;
    ok = checkEquals(static_cast<int>(captured.ccValues.size()), expectedSendCcCount, context + " unique CC count") && ok;

    for (const auto channel : captured.channels)
        ok = checkEquals(channel, expectedMidiChannel, context + " MIDI channel") && ok;

    for (int cc = 20; cc <= 48; ++cc)
        ok = checkPass(captured.ccValues.count(cc) == 1, context + " contains CC" + juce::String(cc)) && ok;

    ok = checkPass(captured.ccValues.count(49) == 1, context + " contains reserved CC49") && ok;

    for (int cc = 50; cc <= 54; ++cc)
        ok = checkPass(captured.ccValues.count(cc) == 1, context + " contains CC" + juce::String(cc)) && ok;

    return ok;
}

bool expectCcValue(const CapturedMidi& captured, int ccNumber, int expectedValue, const juce::String& label)
{
    const auto it = captured.ccValues.find(ccNumber);

    if (it == captured.ccValues.end())
    {
        std::cerr << "[FAIL] " << label << ": missing CC" << ccNumber << std::endl;
        return false;
    }

    return checkEquals(it->second, expectedValue, label + " CC" + juce::String(ccNumber));
}

bool verifyAllOffSend()
{
    OrchConductorAudioProcessor processor;

    processor.requestSendAllOff();

    const auto captured = captureMidi(processor);

    bool ok = true;

    ok = verifyStandardSendShape(captured, "all off send") && ok;

    for (int cc = 20; cc <= 54; ++cc)
    {
        if (cc == 49)
            ok = expectCcValue(captured, cc, 0, "all off reserved") && ok;
        else
            ok = expectCcValue(captured, cc, 0, "all off") && ok;
    }

    return ok;
}

bool verifyManualSectionLowStringsSend()
{
    OrchConductorAudioProcessor processor;

    processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::manualSections));
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::strings,
                                 static_cast<int>(OrchConductorAudioProcessor::Preset::lowStrings));

    processor.requestSendPreset();

    const auto captured = captureMidi(processor);

    bool ok = true;

    ok = verifyStandardSendShape(captured, "manual low strings send") && ok;

    ok = expectCcValue(captured, 49, 0, "manual low strings reserved") && ok;

    ok = expectCcValue(captured, 50, 0, "manual low strings violin I") && ok;
    ok = expectCcValue(captured, 51, 0, "manual low strings violin II") && ok;
    ok = expectCcValue(captured, 52, 64, "manual low strings viola") && ok;
    ok = expectCcValue(captured, 53, 127, "manual low strings cello") && ok;
    ok = expectCcValue(captured, 54, 127, "manual low strings bass") && ok;

    for (int cc = 20; cc <= 48; ++cc)
        ok = expectCcValue(captured, cc, 0, "manual low strings non-string section") && ok;

    return ok;
}

bool verifyManualSectionWoodwindsAndBrassSend()
{
    OrchConductorAudioProcessor processor;

    processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::manualSections));
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::woodwinds, 19); // Full Woodwinds
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::brass, 16);     // Full Brass
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::percussion, 0);
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::strings, 0);

    processor.requestSendPreset();

    const auto captured = captureMidi(processor);

    bool ok = true;

    ok = verifyStandardSendShape(captured, "manual full winds/brass send") && ok;

    for (int cc = 20; cc <= 31; ++cc)
        ok = expectCcValue(captured, cc, 127, "manual full woodwinds") && ok;

    for (int cc = 32; cc <= 42; ++cc)
        ok = expectCcValue(captured, cc, 127, "manual full brass") && ok;

    for (int cc = 43; cc <= 48; ++cc)
        ok = expectCcValue(captured, cc, 0, "manual percussion all off") && ok;

    ok = expectCcValue(captured, 49, 0, "manual full winds/brass reserved") && ok;

    for (int cc = 50; cc <= 54; ++cc)
        ok = expectCcValue(captured, cc, 0, "manual strings all off") && ok;

    return ok;
}

bool verifyCombiOverridesSectionPresets()
{
    OrchConductorAudioProcessor processor;

    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::woodwinds, 19);
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::brass, 16);
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::percussion, 8);
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::strings,
                                 static_cast<int>(OrchConductorAudioProcessor::Preset::tutti));

    processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::soloEnglishHornLament));

    processor.requestSendPreset();

    const auto captured = captureMidi(processor);

    bool ok = true;

    ok = verifyStandardSendShape(captured, "combi override send") && ok;

    // Solo English Horn Lament:
    // CC25 = 127, CC52 = 127, CC53 = 127, CC54 = 64, all other non-reserved CCs zero.
    for (int cc = 20; cc <= 54; ++cc)
    {
        if (cc == 25)
            ok = expectCcValue(captured, cc, 127, "solo English Horn Lament english horn") && ok;
        else if (cc == 52)
            ok = expectCcValue(captured, cc, 127, "solo English Horn Lament viola") && ok;
        else if (cc == 53)
            ok = expectCcValue(captured, cc, 127, "solo English Horn Lament cello") && ok;
        else if (cc == 54)
            ok = expectCcValue(captured, cc, 64, "solo English Horn Lament bass") && ok;
        else
            ok = expectCcValue(captured, cc, 0, "solo English Horn Lament inactive/reserved") && ok;
    }

    return ok;
}

bool verifySendRequestConsumed()
{
    OrchConductorAudioProcessor processor;

    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::strings,
                                 static_cast<int>(OrchConductorAudioProcessor::Preset::lowStrings));

    processor.requestSendPreset();

    const auto first = captureMidi(processor);
    const auto second = captureMidi(processor);

    bool ok = true;

    ok = checkEquals(first.eventCount, expectedSendCcCount, "first requested send event count") && ok;
    ok = checkEquals(second.eventCount, 0, "second send after request consumed event count") && ok;

    return ok;
}

bool verifyProbeDiagnosticsPresentAndNonAuthoritative()
{
    OrchConductorAudioProcessor processor;

    bool ok = true;

    ok = checkPass(processor.getRuntimeJsonPresetProbeDiagnostic().isNotEmpty(),
                   "processor JSON probe diagnostic is present") && ok;
    ok = checkPass(processor.getRuntimeCatalogPayloadEquivalenceProbeDiagnostic().isNotEmpty(),
                   "processor runtime catalog payload equivalence probe diagnostic is present") && ok;
    ok = checkPass(processor.getRuntimeCatalogCoverageAuditDiagnostic().isNotEmpty(),
                   "processor runtime catalog coverage audit diagnostic is present") && ok;
    ok = checkPass(processor.getRuntimeCatalogAuthorityTrialDiagnostic().isNotEmpty(),
                   "processor runtime catalog authority trial diagnostic is present") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    ok = checkPass(processor.getRuntimeJsonPresetProbeDiagnostic().isNotEmpty(),
                   "processor JSON probe reports diagnostic state in ON MIDI regression build") && ok;
    ok = checkPass(processor.getRuntimePresetCatalogAuthorityProbeDiagnostic().isNotEmpty(),
                   "processor runtime catalog authority probe diagnostic is present in ON MIDI regression build") && ok;
    ok = checkPass(processor.wasRuntimePresetCatalogAuthorityProbeReady(),
                   "processor runtime catalog authority probe ready in ON MIDI regression build") && ok;
    ok = checkPass(processor.doesRuntimePresetCatalogAuthorityProbeHaveExpectedFactoryShape(),
                   "processor runtime catalog authority probe has expected factory shape in ON MIDI regression build") && ok;

    if (processor.wasRuntimeJsonPresetProbeLoaded())
    {
        ok = checkPass(! processor.doesRuntimeJsonPresetProbeRequireFallback(),
                       "processor JSON probe loaded without fallback in ON MIDI regression build") && ok;
    }
    else
    {
        ok = checkPass(processor.doesRuntimeJsonPresetProbeRequireFallback(),
                       "processor JSON probe falls back safely in ON MIDI regression build") && ok;
    }

    if (processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback())
    {
        ok = checkPass(processor.doesRuntimeJsonPresetProbeRequireFallback(),
                       "processor runtime catalog authority probe fallback follows source fallback in ON MIDI regression build") && ok;
        ok = checkPass(! processor.wasRuntimeCatalogPayloadEquivalenceProbeRun(),
                       "processor runtime catalog payload equivalence probe does not run against fallback catalog in ON MIDI regression build") && ok;
        ok = checkPass(! processor.didRuntimeCatalogPayloadEquivalenceProbePass(),
                       "processor runtime catalog payload equivalence probe does not pass against fallback catalog in ON MIDI regression build") && ok;
        ok = checkPass(processor.wasRuntimeCatalogPayloadEquivalenceProbeBlockedByFallback(),
                       "processor runtime catalog payload equivalence probe reports fallback block in ON MIDI regression build") && ok;

        ok = checkPass(! processor.wasRuntimeCatalogCoverageAuditRun(),
                       "processor runtime catalog coverage audit does not run against fallback catalog in ON MIDI regression build") && ok;
        ok = checkPass(! processor.didRuntimeCatalogCoverageAuditPass(),
                       "processor runtime catalog coverage audit does not pass against fallback catalog in ON MIDI regression build") && ok;
        ok = checkPass(processor.wasRuntimeCatalogCoverageAuditBlockedByFallback(),
                       "processor runtime catalog coverage audit reports fallback block in ON MIDI regression build") && ok;

        ok = checkPass(! processor.wasRuntimeCatalogAuthorityTrialRun(),
                       "processor runtime catalog authority trial does not run against fallback catalog in ON MIDI regression build") && ok;
        ok = checkPass(! processor.didRuntimeCatalogAuthorityTrialPass(),
                       "processor runtime catalog authority trial does not pass against fallback catalog in ON MIDI regression build") && ok;
        ok = checkPass(processor.wasRuntimeCatalogAuthorityTrialBlocked(),
                       "processor runtime catalog authority trial reports fallback block in ON MIDI regression build") && ok;
    }
    else
    {
        ok = checkPass(processor.wasRuntimeJsonPresetProbeLoaded(),
                       "processor runtime catalog authority probe uses loaded source in ON MIDI regression build") && ok;
        ok = checkPass(processor.wasRuntimeCatalogPayloadEquivalenceProbeRun(),
                       "processor runtime catalog payload equivalence probe runs against source-backed catalog in ON MIDI regression build") && ok;
        ok = checkPass(processor.didRuntimeCatalogPayloadEquivalenceProbePass(),
                       "processor runtime catalog payload equivalence probe passes in ON MIDI regression build") && ok;
        ok = checkPass(! processor.wasRuntimeCatalogPayloadEquivalenceProbeBlockedByFallback(),
                       "processor runtime catalog payload equivalence probe is not fallback-blocked in ON MIDI regression build") && ok;

        ok = checkPass(processor.wasRuntimeCatalogCoverageAuditRun(),
                       "processor runtime catalog coverage audit runs against source-backed catalog in ON MIDI regression build") && ok;
        ok = checkPass(processor.didRuntimeCatalogCoverageAuditPass(),
                       "processor runtime catalog coverage audit passes in ON MIDI regression build") && ok;
        ok = checkPass(! processor.wasRuntimeCatalogCoverageAuditBlockedByFallback(),
                       "processor runtime catalog coverage audit is not fallback-blocked in ON MIDI regression build") && ok;

    #if ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL
        ok = checkPass(processor.wasRuntimeCatalogAuthorityTrialRun(),
                       "processor runtime catalog authority trial runs in authority-trial ON MIDI regression build") && ok;
        ok = checkPass(processor.didRuntimeCatalogAuthorityTrialPass(),
                       "processor runtime catalog authority trial passes in authority-trial ON MIDI regression build") && ok;
        ok = checkPass(! processor.wasRuntimeCatalogAuthorityTrialBlocked(),
                       "processor runtime catalog authority trial is not blocked in authority-trial ON MIDI regression build") && ok;
    #else
        ok = checkPass(! processor.wasRuntimeCatalogAuthorityTrialRun(),
                       "processor runtime catalog authority trial remains inactive when authority-trial gate is OFF in ON MIDI regression build") && ok;
        ok = checkPass(! processor.didRuntimeCatalogAuthorityTrialPass(),
                       "processor runtime catalog authority trial does not pass when authority-trial gate is OFF in ON MIDI regression build") && ok;
        ok = checkPass(processor.wasRuntimeCatalogAuthorityTrialBlocked(),
                       "processor runtime catalog authority trial is blocked when authority-trial gate is OFF in ON MIDI regression build") && ok;
    #endif
    }
#else
    ok = checkPass(! processor.wasRuntimeJsonPresetProbeLoaded(),
                   "processor JSON probe not loaded in OFF MIDI regression build") && ok;

    ok = checkPass(processor.doesRuntimeJsonPresetProbeRequireFallback(),
                   "processor JSON probe requires fallback in OFF MIDI regression build") && ok;
    ok = checkPass(! processor.wasRuntimeCatalogPayloadEquivalenceProbeRun(),
                   "processor runtime catalog payload equivalence probe inactive in OFF MIDI regression build") && ok;
    ok = checkPass(! processor.didRuntimeCatalogPayloadEquivalenceProbePass(),
                   "processor runtime catalog payload equivalence probe does not pass in OFF MIDI regression build") && ok;
    ok = checkPass(processor.wasRuntimeCatalogPayloadEquivalenceProbeBlockedByFallback(),
                   "processor runtime catalog payload equivalence probe blocked in OFF MIDI regression build") && ok;

    ok = checkPass(! processor.wasRuntimeCatalogCoverageAuditRun(),
                   "processor runtime catalog coverage audit inactive in OFF MIDI regression build") && ok;
    ok = checkPass(! processor.didRuntimeCatalogCoverageAuditPass(),
                   "processor runtime catalog coverage audit does not pass in OFF MIDI regression build") && ok;
    ok = checkPass(processor.wasRuntimeCatalogCoverageAuditBlockedByFallback(),
                   "processor runtime catalog coverage audit blocked in OFF MIDI regression build") && ok;

    ok = checkPass(! processor.wasRuntimeCatalogAuthorityTrialRun(),
                   "processor runtime catalog authority trial inactive in OFF MIDI regression build") && ok;
    ok = checkPass(! processor.didRuntimeCatalogAuthorityTrialPass(),
                   "processor runtime catalog authority trial does not pass in OFF MIDI regression build") && ok;
    ok = checkPass(processor.wasRuntimeCatalogAuthorityTrialBlocked(),
                   "processor runtime catalog authority trial blocked in OFF MIDI regression build") && ok;
#endif

    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::strings,
                                 static_cast<int>(OrchConductorAudioProcessor::Preset::lowStrings));
    processor.requestSendPreset();

    const auto captured = captureMidi(processor);

    ok = expectCcValue(captured, 52, 64, "probe diagnostic non-authoritative low strings viola") && ok;
    ok = expectCcValue(captured, 53, 127, "probe diagnostic non-authoritative low strings cello") && ok;
    ok = expectCcValue(captured, 54, 127, "probe diagnostic non-authoritative low strings bass") && ok;

    return ok;
}

} // namespace

int main()
{
    std::cout << "OrchConductor processor MIDI regression check" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    std::cout << "[INFO] Runtime JSON preset feature gate: ON" << std::endl;
#else
    std::cout << "[INFO] Runtime JSON preset feature gate: OFF" << std::endl;
#endif

    bool ok = true;

    ok = verifyNoMidiWithoutRequest() && ok;
    ok = verifyAllOffSend() && ok;
    ok = verifyManualSectionLowStringsSend() && ok;
    ok = verifyManualSectionWoodwindsAndBrassSend() && ok;
    ok = verifyCombiOverridesSectionPresets() && ok;
    ok = verifySendRequestConsumed() && ok;
    ok = verifyProbeDiagnosticsPresentAndNonAuthoritative() && ok;

    if (! ok)
        return fail("Processor MIDI regression verification failed.");

    std::cout << "-----------------------------------------------" << std::endl;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    std::cout << "[PASS] Processor MIDI regression verification completed successfully with runtime JSON probe ON." << std::endl;
#else
    std::cout << "[PASS] Processor MIDI regression verification completed successfully with runtime JSON probe OFF." << std::endl;
#endif

    std::cout << "[PASS] Hardcoded MIDI behavior remains authoritative." << std::endl;

    std::cout << "[PASS] Phase 5B processor-side runtime catalog authority probe preserved MIDI behavior." << std::endl;
    std::cout << "[PASS] Phase 5C processor runtime catalog payload equivalence probe preserved MIDI behavior." << std::endl;
    std::cout << "[PASS] Phase 5H runtime catalog authority trial invariants preserved MIDI behavior." << std::endl;

    return 0;
}






