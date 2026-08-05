#include <JuceHeader.h>

#include "../Source/OrchConductorProcessor.h"

#include <iostream>

namespace
{

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

bool verifyHardcodedBehaviorStillAvailable(OrchConductorAudioProcessor& processor)
{
    bool ok = true;

    ok = checkEquals(processor.getCombiPresetId(), 0, "initial combi preset id") && ok;
    ok = checkEquals(processor.getSectionPresetId(OrchConductorAudioProcessor::Section::woodwinds), 0, "initial woodwinds preset id") && ok;
    ok = checkEquals(processor.getSectionPresetId(OrchConductorAudioProcessor::Section::brass), 0, "initial brass preset id") && ok;
    ok = checkEquals(processor.getSectionPresetId(OrchConductorAudioProcessor::Section::percussion), 0, "initial percussion preset id") && ok;
    ok = checkEquals(processor.getSectionPresetId(OrchConductorAudioProcessor::Section::strings), 0, "initial strings preset id") && ok;

    ok = checkPass(processor.getPresetName() == "All Off", "hardcoded strings preset name remains All Off") && ok;
    ok = checkPass(processor.getCombiPresetName() == "Manual Sections", "hardcoded combi preset name remains Manual Sections") && ok;

    processor.setPreset(OrchConductorAudioProcessor::Preset::lowStrings);

    const auto violaRow = processor.getOutputRow(2);
    const auto celloRow = processor.getOutputRow(3);
    const auto bassRow = processor.getOutputRow(4);

    ok = checkEquals(violaRow.ccNumber, 52, "low strings viola CC") && ok;
    ok = checkEquals(violaRow.value, 64, "low strings viola value remains hardcoded") && ok;
    ok = checkEquals(celloRow.ccNumber, 53, "low strings cello CC") && ok;
    ok = checkEquals(celloRow.value, 127, "low strings cello value remains hardcoded") && ok;
    ok = checkEquals(bassRow.ccNumber, 54, "low strings bass CC") && ok;
    ok = checkEquals(bassRow.value, 127, "low strings bass value remains hardcoded") && ok;

    processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::soloEnglishHornLament));

    ok = checkPass(processor.isCombiModeActive(), "hardcoded combi mode remains available") && ok;

    const auto bassInCombi = processor.getOutputRow(4);
    ok = checkEquals(bassInCombi.ccNumber, 54, "solo English Horn Lament bass CC") && ok;
    ok = checkEquals(bassInCombi.value, 64, "solo English Horn Lament bass value remains hardcoded") && ok;

    return ok;
}

bool verifyProcessorRuntimeCatalogAuthorityProbe(OrchConductorAudioProcessor& processor)
{
    bool ok = true;

    ok = checkPass(processor.getRuntimePresetCatalogAuthorityProbeDiagnostic().isNotEmpty(),
                   "processor runtime catalog authority probe diagnostic message is present") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    ok = checkPass(processor.wasRuntimePresetCatalogAuthorityProbeReady(),
                   "processor runtime catalog authority probe candidate is ready with runtime JSON ON") && ok;
    ok = checkPass(processor.doesRuntimePresetCatalogAuthorityProbeHaveExpectedFactoryShape(),
                   "processor runtime catalog authority probe candidate has expected factory shape with runtime JSON ON") && ok;

    if (processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback())
    {
        ok = checkPass(processor.doesRuntimeJsonPresetProbeRequireFallback(),
                       "processor runtime catalog authority probe falls back safely when source fallback is required with runtime JSON ON") && ok;
    }
    else
    {
        ok = checkPass(processor.wasRuntimeJsonPresetProbeLoaded(),
                       "processor runtime catalog authority probe uses loaded source when fallback is not required with runtime JSON ON") && ok;
    }
#else
    ok = checkPass(! processor.wasRuntimePresetCatalogAuthorityProbeReady(),
                   "processor runtime catalog authority probe inactive with runtime JSON OFF") && ok;
    ok = checkPass(processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback(),
                   "processor runtime catalog authority probe requires fallback with runtime JSON OFF") && ok;
    ok = checkPass(! processor.doesRuntimePresetCatalogAuthorityProbeHaveExpectedFactoryShape(),
                   "processor runtime catalog authority probe has no authority-candidate shape with runtime JSON OFF") && ok;
    ok = checkPass(! processor.wasRuntimeJsonPresetProbeLoaded(),
                   "processor runtime catalog authority probe source remains unloaded with runtime JSON OFF") && ok;
#endif

    return ok;
}
bool verifyProcessorRuntimeCatalogCoverageAudit(OrchConductorAudioProcessor& processor)
{
    bool ok = true;

    ok = checkPass(processor.getRuntimeCatalogCoverageAuditDiagnostic().isNotEmpty(),
                   "processor runtime catalog coverage audit diagnostic message is present") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    if (processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback())
    {
        ok = checkPass(! processor.wasRuntimeCatalogCoverageAuditRun(),
                       "processor runtime catalog coverage audit does not run against fallback catalog") && ok;
        ok = checkPass(processor.wasRuntimeCatalogCoverageAuditBlockedByFallback(),
                       "processor runtime catalog coverage audit reports fallback block") && ok;
    }
    else
    {
        ok = checkPass(processor.wasRuntimeCatalogCoverageAuditRun(),
                       "processor runtime catalog coverage audit runs against source-backed catalog") && ok;
        ok = checkPass(processor.didRuntimeCatalogCoverageAuditPass(),
                       "processor runtime catalog coverage audit passes for expected factory catalog shape") && ok;
        ok = checkPass(! processor.wasRuntimeCatalogCoverageAuditBlockedByFallback(),
                       "processor runtime catalog coverage audit is not fallback-blocked with source-backed catalog") && ok;
    }
#else
    ok = checkPass(! processor.wasRuntimeCatalogCoverageAuditRun(),
                   "processor runtime catalog coverage audit inactive with runtime JSON OFF") && ok;
    ok = checkPass(! processor.didRuntimeCatalogCoverageAuditPass(),
                   "processor runtime catalog coverage audit does not report pass with runtime JSON OFF") && ok;
    ok = checkPass(processor.wasRuntimeCatalogCoverageAuditBlockedByFallback(),
                   "processor runtime catalog coverage audit blocked with runtime JSON OFF") && ok;
#endif

    return ok;
}

bool verifyProcessorRuntimeCatalogPayloadEquivalenceProbe(OrchConductorAudioProcessor& processor)
{
    bool ok = true;

    ok = checkPass(processor.getRuntimeCatalogPayloadEquivalenceProbeDiagnostic().isNotEmpty(),
                   "processor runtime catalog payload equivalence probe diagnostic message is present") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    if (processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback())
    {
        ok = checkPass(! processor.wasRuntimeCatalogPayloadEquivalenceProbeRun(),
                       "processor runtime catalog payload equivalence probe does not run against fallback catalog") && ok;
        ok = checkPass(processor.wasRuntimeCatalogPayloadEquivalenceProbeBlockedByFallback(),
                       "processor runtime catalog payload equivalence probe reports fallback block") && ok;
    }
    else
    {
        ok = checkPass(processor.wasRuntimeCatalogPayloadEquivalenceProbeRun(),
                       "processor runtime catalog payload equivalence probe runs against source-backed catalog") && ok;
        ok = checkPass(processor.didRuntimeCatalogPayloadEquivalenceProbePass(),
                       "processor runtime catalog payload equivalence probe passes for broadened sentinel hardcoded presets") && ok;
        ok = checkPass(! processor.wasRuntimeCatalogPayloadEquivalenceProbeBlockedByFallback(),
                       "processor runtime catalog payload equivalence probe is not fallback-blocked with source-backed catalog") && ok;
    }
#else
    ok = checkPass(! processor.wasRuntimeCatalogPayloadEquivalenceProbeRun(),
                   "processor runtime catalog payload equivalence probe inactive with runtime JSON OFF") && ok;
    ok = checkPass(! processor.didRuntimeCatalogPayloadEquivalenceProbePass(),
                   "processor runtime catalog payload equivalence probe does not report pass with runtime JSON OFF") && ok;
    ok = checkPass(processor.wasRuntimeCatalogPayloadEquivalenceProbeBlockedByFallback(),
                   "processor runtime catalog payload equivalence probe blocked with runtime JSON OFF") && ok;
#endif

    return ok;
}
} // namespace

int main()
{
    std::cout << "OrchConductor processor runtime catalog payload equivalence probe check" << std::endl;
    std::cout << "---------------------------------------------------------------------" << std::endl;

    OrchConductorAudioProcessor processor;

    std::cout << "[INFO] Source probe diagnostic: " << processor.getRuntimeJsonPresetProbeDiagnostic() << std::endl;
    std::cout << "[INFO] Catalog authority probe diagnostic: " << processor.getRuntimePresetCatalogAuthorityProbeDiagnostic() << std::endl;
    std::cout << "[INFO] Payload equivalence probe diagnostic: " << processor.getRuntimeCatalogPayloadEquivalenceProbeDiagnostic() << std::endl;
    std::cout << "[INFO] Coverage audit diagnostic: " << processor.getRuntimeCatalogCoverageAuditDiagnostic() << std::endl;

    bool ok = true;

    ok = checkPass(processor.getRuntimeJsonPresetProbeDiagnostic().isNotEmpty(),
                   "processor JSON probe diagnostic message is present") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    ok = checkPass(processor.getRuntimeJsonPresetProbeDiagnostic().isNotEmpty(),
                   "processor JSON probe reports diagnostic state when feature gate is ON") && ok;

    if (processor.wasRuntimeJsonPresetProbeLoaded())
    {
        ok = checkPass(! processor.doesRuntimeJsonPresetProbeRequireFallback(),
                       "processor JSON probe loaded without fallback when embedded JSON is usable") && ok;
    }
    else
    {
        ok = checkPass(processor.doesRuntimeJsonPresetProbeRequireFallback(),
                       "processor JSON probe falls back safely when embedded JSON is unavailable or malformed") && ok;
    }
#else
    ok = checkPass(! processor.wasRuntimeJsonPresetProbeLoaded(),
                   "processor JSON probe does not load when feature gate is OFF") && ok;

    ok = checkPass(processor.doesRuntimeJsonPresetProbeRequireFallback(),
                   "processor JSON probe requires fallback when feature gate is OFF") && ok;
#endif

    ok = verifyProcessorRuntimeCatalogAuthorityProbe(processor) && ok;
    ok = verifyProcessorRuntimeCatalogPayloadEquivalenceProbe(processor) && ok;
    ok = verifyProcessorRuntimeCatalogCoverageAudit(processor) && ok;
    ok = verifyHardcodedBehaviorStillAvailable(processor) && ok;

    if (! ok)
        return fail("Processor-side runtime catalog authority probe verification failed.");

    std::cout << "---------------------------------------------------------------------" << std::endl;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    std::cout << "[PASS] Processor-side runtime catalog authority probe ON verification completed successfully." << std::endl;
#else
    std::cout << "[PASS] Processor-side runtime catalog authority probe OFF verification completed successfully." << std::endl;
#endif

    std::cout << "[PASS] Hardcoded preset behavior remains authoritative." << std::endl;
    std::cout << "[PASS] Processor runtime catalog authority probe remains passive." << std::endl;
    std::cout << "[PASS] Processor runtime catalog payload equivalence probe remains passive." << std::endl;
    std::cout << "[PASS] Processor runtime catalog coverage audit remains passive." << std::endl;
    std::cout << "[PASS] Phase 5D broadened processor runtime catalog payload equivalence coverage satisfied." << std::endl;
    std::cout << "[PASS] Phase 5E runtime catalog coverage audit satisfied." << std::endl;

    return 0;
}





