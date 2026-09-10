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


bool verifyProcessorRuntimeCatalogAuthorityTrialDiagnostic(OrchConductorAudioProcessor& processor)
{
    bool ok = true;

    ok = checkPass(processor.getRuntimeCatalogAuthorityTrialDiagnostic().isNotEmpty(),
                   "processor runtime catalog authority trial diagnostic message is present") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    if (processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback())
    {
        ok = checkPass(! processor.wasRuntimeCatalogAuthorityTrialRun(),
                       "processor runtime catalog authority trial does not run against fallback catalog") && ok;
        ok = checkPass(processor.wasRuntimeCatalogAuthorityTrialBlocked(),
                       "processor runtime catalog authority trial reports fallback block") && ok;
        ok = checkPass(! processor.didRuntimeCatalogAuthorityTrialPass(),
                       "processor runtime catalog authority trial does not report pass when fallback-blocked") && ok;
    }
    else
    {
    #if ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL
        ok = checkPass(processor.wasRuntimeCatalogAuthorityTrialRun(),
                       "processor runtime catalog authority trial runs against source-backed catalog when enabled") && ok;
        ok = checkPass(processor.didRuntimeCatalogAuthorityTrialPass(),
                       "processor runtime catalog authority trial passes for selected factory authority sentinels") && ok;
        ok = checkPass(! processor.wasRuntimeCatalogAuthorityTrialBlocked(),
                       "processor runtime catalog authority trial is not blocked with source-backed catalog when enabled") && ok;
    #else
        ok = checkPass(! processor.wasRuntimeCatalogAuthorityTrialRun(),
                       "processor runtime catalog authority trial remains inactive when authority trial feature gate is OFF") && ok;
        ok = checkPass(! processor.didRuntimeCatalogAuthorityTrialPass(),
                       "processor runtime catalog authority trial does not report pass when authority trial feature gate is OFF") && ok;
        ok = checkPass(processor.wasRuntimeCatalogAuthorityTrialBlocked(),
                       "processor runtime catalog authority trial reports blocked when authority trial feature gate is OFF") && ok;
    #endif
    }
#else
    ok = checkPass(! processor.wasRuntimeCatalogAuthorityTrialRun(),
                   "processor runtime catalog authority trial inactive with runtime JSON OFF") && ok;
    ok = checkPass(! processor.didRuntimeCatalogAuthorityTrialPass(),
                   "processor runtime catalog authority trial does not report pass with runtime JSON OFF") && ok;
    ok = checkPass(processor.wasRuntimeCatalogAuthorityTrialBlocked(),
                   "processor runtime catalog authority trial blocked with runtime JSON OFF") && ok;
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

juce::AudioProcessorParameterWithID* findParameterWithId(OrchConductorAudioProcessor& processor,
                                                         const juce::String& parameterId)
{
    for (auto* parameter : processor.getParameters())
    {
        if (auto* parameterWithId = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
        {
            if (parameterWithId->paramID == parameterId)
                return parameterWithId;
        }
    }

    return nullptr;
}

juce::AudioParameterBool* findBoolParameterWithId(OrchConductorAudioProcessor& processor,
                                                  const juce::String& parameterId)
{
    return dynamic_cast<juce::AudioParameterBool*>(
        findParameterWithId(processor, parameterId));
}
juce::AudioParameterInt* findIntParameterWithId(OrchConductorAudioProcessor& processor,
                                                const juce::String& parameterId)
{
    return dynamic_cast<juce::AudioParameterInt*>(
        findParameterWithId(processor, parameterId));
}
bool setBoolParameterValueById(OrchConductorAudioProcessor& processor,
                               const juce::String& parameterId,
                               bool value)
{
    auto* parameter = findBoolParameterWithId(processor, parameterId);

    if (parameter == nullptr)
        return false;

    parameter->setValueNotifyingHost(value ? 1.0f : 0.0f);
    return true;
}

bool setIntParameterValueById(OrchConductorAudioProcessor& processor,
                              const juce::String& parameterId,
                              int value)
{
    auto* parameter = findIntParameterWithId(processor, parameterId);

    if (parameter == nullptr)
        return false;

    parameter->setValueNotifyingHost(
        parameter->convertTo0to1(static_cast<float>(value)));

    return true;
}
bool verifyProcessorStatePersistenceWithRuntimeCatalogAuthority()
{
    bool ok = true;

    OrchConductorAudioProcessor source;

    source.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::soloEnglishHornLament));
    source.setSectionPresetId(OrchConductorAudioProcessor::Section::woodwinds, 19);
    source.setSectionPresetId(OrchConductorAudioProcessor::Section::brass, 16);
    source.setSectionPresetId(OrchConductorAudioProcessor::Section::percussion, 8);
    source.setSectionPresetId(OrchConductorAudioProcessor::Section::strings,
                              static_cast<int>(OrchConductorAudioProcessor::Preset::tutti));
    source.setSendOnPresetChange(true);

    juce::MemoryBlock state;
    source.getStateInformation(state);

    OrchConductorAudioProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    ok = checkEquals(restored.getCombiPresetId(),
                     static_cast<int>(OrchConductorAudioProcessor::CombiPreset::soloEnglishHornLament),
                     "restored combi preset id") && ok;

    ok = checkEquals(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::woodwinds),
                     19,
                     "restored woodwinds preset id") && ok;

    ok = checkEquals(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::brass),
                     16,
                     "restored brass preset id") && ok;

    ok = checkEquals(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::percussion),
                     8,
                     "restored percussion preset id") && ok;

    ok = checkEquals(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::strings),
                     static_cast<int>(OrchConductorAudioProcessor::Preset::tutti),
                     "restored strings preset id") && ok;

    ok = checkPass(restored.getSendOnPresetChange(),
                   "restored send-on-preset-change flag") && ok;

    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;
    restored.processBlock(buffer, midi);

    ok = checkPass(restored.getSendOnPresetChange(),
                   "restored send-on-preset-change flag survives automation sync") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    if (restored.isRuntimePresetCatalogAuthorityActive())
    {
        ok = checkPass(restored.getRuntimePresetCatalogAuthorityStatus().contains("Runtime JSON: Active"),
                       "restored processor reports active runtime catalog authority") && ok;

        ok = checkPass(restored.getCombiPresetName().isNotEmpty(),
                       "restored runtime-authority combi label is non-empty") && ok;

        ok = checkPass(restored.getSectionPresetLabel(OrchConductorAudioProcessor::Section::woodwinds, 19).isNotEmpty(),
                       "restored runtime-authority woodwinds label is non-empty") && ok;

        ok = checkPass(restored.getSectionPresetLabel(OrchConductorAudioProcessor::Section::brass, 16).isNotEmpty(),
                       "restored runtime-authority brass label is non-empty") && ok;

        ok = checkPass(restored.getSectionPresetLabel(OrchConductorAudioProcessor::Section::percussion, 8).isNotEmpty(),
                       "restored runtime-authority percussion label is non-empty") && ok;

        ok = checkPass(restored.getPresetName().isNotEmpty(),
                       "restored runtime-authority strings label is non-empty") && ok;
    }
    else
    {
        ok = checkPass(restored.getRuntimePresetCatalogAuthorityStatus().isNotEmpty(),
                       "restored processor reports non-empty runtime catalog authority status when inactive") && ok;
    }
#else
    ok = checkPass(! restored.isRuntimePresetCatalogAuthorityActive(),
                   "restored processor runtime catalog authority remains inactive with runtime JSON OFF") && ok;
#endif

    return ok;
}

bool verifySendOnPresetChangeAutomationPersistenceRoundTrip()
{
    bool ok = true;

    OrchConductorAudioProcessor automated;

    ok = checkPass(! automated.getSendOnPresetChange(),
                   "automation persistence source send-on-preset-change starts disabled") && ok;

    auto* automatedParameter = findBoolParameterWithId(automated, "sendOnPresetChange");

    ok = checkPass(automatedParameter != nullptr,
                   "send-on-preset-change automation parameter is discoverable by id") && ok;

    if (automatedParameter == nullptr)
        return false;

    ok = checkPass(! automatedParameter->get(),
                   "send-on-preset-change automation parameter starts disabled") && ok;

    automatedParameter->setValueNotifyingHost(1.0f);

    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;
    automated.processBlock(buffer, midi);

    ok = checkPass(automated.getSendOnPresetChange(),
                   "automation-updated send-on-preset-change syncs into processor state") && ok;

    ok = checkPass(automatedParameter->get(),
                   "automation-updated send-on-preset-change parameter remains enabled after sync") && ok;

    juce::MemoryBlock state;
    automated.getStateInformation(state);

    OrchConductorAudioProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    ok = checkPass(restored.getSendOnPresetChange(),
                   "automation-updated send-on-preset-change persists through restore") && ok;

    auto* restoredParameter = findBoolParameterWithId(restored, "sendOnPresetChange");

    ok = checkPass(restoredParameter != nullptr,
                   "restored send-on-preset-change automation parameter is discoverable by id") && ok;

    if (restoredParameter == nullptr)
        return false;

    ok = checkPass(restoredParameter->get(),
                   "restored send-on-preset-change automation parameter mirrors persisted state") && ok;

    restored.processBlock(buffer, midi);

    ok = checkPass(restored.getSendOnPresetChange(),
                   "restored automation-updated send-on-preset-change survives processBlock sync") && ok;

    ok = checkPass(restoredParameter->get(),
                   "restored automation-updated send-on-preset-change parameter survives processBlock sync") && ok;

    return ok;
}
bool verifyPresetAutomationPersistenceRoundTrip()
{
    bool ok = true;

    constexpr int combiPresetId = 28;
    constexpr int woodwindsPresetId = 19;
    constexpr int brassPresetId = 16;
    constexpr int percussionPresetId = 8;
    constexpr int stringsPresetId = 13;

    OrchConductorAudioProcessor automated;

    ok = checkPass(findIntParameterWithId(automated, "combiPreset") != nullptr,
                   "combi preset automation parameter is discoverable by id") && ok;
    ok = checkPass(findIntParameterWithId(automated, "woodwindsPreset") != nullptr,
                   "woodwinds preset automation parameter is discoverable by id") && ok;
    ok = checkPass(findIntParameterWithId(automated, "brassPreset") != nullptr,
                   "brass preset automation parameter is discoverable by id") && ok;
    ok = checkPass(findIntParameterWithId(automated, "percussionPreset") != nullptr,
                   "percussion preset automation parameter is discoverable by id") && ok;
    ok = checkPass(findIntParameterWithId(automated, "stringsPreset") != nullptr,
                   "strings preset automation parameter is discoverable by id") && ok;

    ok = checkPass(setIntParameterValueById(automated, "combiPreset", combiPresetId),
                   "combi preset automation parameter accepts target value") && ok;
    ok = checkPass(setIntParameterValueById(automated, "woodwindsPreset", woodwindsPresetId),
                   "woodwinds preset automation parameter accepts target value") && ok;
    ok = checkPass(setIntParameterValueById(automated, "brassPreset", brassPresetId),
                   "brass preset automation parameter accepts target value") && ok;
    ok = checkPass(setIntParameterValueById(automated, "percussionPreset", percussionPresetId),
                   "percussion preset automation parameter accepts target value") && ok;
    ok = checkPass(setIntParameterValueById(automated, "stringsPreset", stringsPresetId),
                   "strings preset automation parameter accepts target value") && ok;

    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;
    automated.processBlock(buffer, midi);

    ok = checkPass(automated.getCombiPresetId() == combiPresetId,
                   "automation-updated combi preset syncs into processor state") && ok;
    ok = checkPass(automated.getSectionPresetId(OrchConductorAudioProcessor::Section::woodwinds) == woodwindsPresetId,
                   "automation-updated woodwinds preset syncs into processor state") && ok;
    ok = checkPass(automated.getSectionPresetId(OrchConductorAudioProcessor::Section::brass) == brassPresetId,
                   "automation-updated brass preset syncs into processor state") && ok;
    ok = checkPass(automated.getSectionPresetId(OrchConductorAudioProcessor::Section::percussion) == percussionPresetId,
                   "automation-updated percussion preset syncs into processor state") && ok;
    ok = checkPass(automated.getSectionPresetId(OrchConductorAudioProcessor::Section::strings) == stringsPresetId,
                   "automation-updated strings preset syncs into processor state") && ok;

    juce::MemoryBlock state;
    automated.getStateInformation(state);

    OrchConductorAudioProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    ok = checkPass(restored.getCombiPresetId() == combiPresetId,
                   "automation-updated combi preset persists through restore") && ok;
    ok = checkPass(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::woodwinds) == woodwindsPresetId,
                   "automation-updated woodwinds preset persists through restore") && ok;
    ok = checkPass(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::brass) == brassPresetId,
                   "automation-updated brass preset persists through restore") && ok;
    ok = checkPass(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::percussion) == percussionPresetId,
                   "automation-updated percussion preset persists through restore") && ok;
    ok = checkPass(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::strings) == stringsPresetId,
                   "automation-updated strings preset persists through restore") && ok;

    auto* restoredCombiParameter = findIntParameterWithId(restored, "combiPreset");
    auto* restoredWoodwindsParameter = findIntParameterWithId(restored, "woodwindsPreset");
    auto* restoredBrassParameter = findIntParameterWithId(restored, "brassPreset");
    auto* restoredPercussionParameter = findIntParameterWithId(restored, "percussionPreset");
    auto* restoredStringsParameter = findIntParameterWithId(restored, "stringsPreset");

    ok = checkPass(restoredCombiParameter != nullptr && restoredCombiParameter->get() == combiPresetId,
                   "restored combi automation parameter mirrors persisted preset") && ok;
    ok = checkPass(restoredWoodwindsParameter != nullptr && restoredWoodwindsParameter->get() == woodwindsPresetId,
                   "restored woodwinds automation parameter mirrors persisted preset") && ok;
    ok = checkPass(restoredBrassParameter != nullptr && restoredBrassParameter->get() == brassPresetId,
                   "restored brass automation parameter mirrors persisted preset") && ok;
    ok = checkPass(restoredPercussionParameter != nullptr && restoredPercussionParameter->get() == percussionPresetId,
                   "restored percussion automation parameter mirrors persisted preset") && ok;
    ok = checkPass(restoredStringsParameter != nullptr && restoredStringsParameter->get() == stringsPresetId,
                   "restored strings automation parameter mirrors persisted preset") && ok;

    ok = checkPass(restored.isRuntimePresetCatalogAuthorityActive(),
                   "automation-restored processor reports active runtime catalog authority") && ok;
    ok = checkPass(restored.getCombiPresetLabel(combiPresetId).isNotEmpty(),
                   "automation-restored runtime-authority combi label is non-empty") && ok;
    ok = checkPass(restored.getSectionPresetLabel(OrchConductorAudioProcessor::Section::woodwinds, woodwindsPresetId).isNotEmpty(),
                   "automation-restored runtime-authority woodwinds label is non-empty") && ok;
    ok = checkPass(restored.getSectionPresetLabel(OrchConductorAudioProcessor::Section::brass, brassPresetId).isNotEmpty(),
                   "automation-restored runtime-authority brass label is non-empty") && ok;
    ok = checkPass(restored.getSectionPresetLabel(OrchConductorAudioProcessor::Section::percussion, percussionPresetId).isNotEmpty(),
                   "automation-restored runtime-authority percussion label is non-empty") && ok;
    ok = checkPass(restored.getSectionPresetLabel(OrchConductorAudioProcessor::Section::strings, stringsPresetId).isNotEmpty(),
                   "automation-restored runtime-authority strings label is non-empty") && ok;

    restored.processBlock(buffer, midi);

    ok = checkPass(restored.getCombiPresetId() == combiPresetId,
                   "restored automation-updated combi preset survives processBlock sync") && ok;
    ok = checkPass(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::woodwinds) == woodwindsPresetId,
                   "restored automation-updated woodwinds preset survives processBlock sync") && ok;
    ok = checkPass(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::brass) == brassPresetId,
                   "restored automation-updated brass preset survives processBlock sync") && ok;
    ok = checkPass(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::percussion) == percussionPresetId,
                   "restored automation-updated percussion preset survives processBlock sync") && ok;
    ok = checkPass(restored.getSectionPresetId(OrchConductorAudioProcessor::Section::strings) == stringsPresetId,
                   "restored automation-updated strings preset survives processBlock sync") && ok;

    return ok;
}
bool verifyAutomationTriggeredSendRequestBehavior()
{
    bool ok = true;

    constexpr int firstCombiPresetId = 28;
    constexpr int secondCombiPresetId = 27;
    constexpr int disabledCombiPresetId = 26;

    OrchConductorAudioProcessor processor;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;

    ok = checkPass(processor.getSendOnPresetChange() == false,
                   "automation send behavior starts with send-on-preset-change disabled") && ok;

    ok = checkPass(setBoolParameterValueById(processor, "sendOnPresetChange", true),
                   "automation send behavior can enable send-on-preset-change by parameter id") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getSendOnPresetChange(),
                   "automation send behavior syncs enabled send-on-preset-change before preset automation") && ok;
    ok = checkPass(midi.getNumEvents() == 0,
                   "enabling send-on-preset-change alone does not emit preset MIDI") && ok;

    midi.clear();

    ok = checkPass(setIntParameterValueById(processor, "combiPreset", firstCombiPresetId),
                   "automation send behavior can set first combi preset by parameter id") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getCombiPresetId() == firstCombiPresetId,
                   "automation send behavior syncs first combi preset into state") && ok;
    ok = checkPass(midi.getNumEvents() > 0,
                   "automation-changing combi preset emits MIDI when send-on-preset-change is enabled") && ok;

    const int firstSendEventCount = midi.getNumEvents();
    midi.clear();

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getCombiPresetId() == firstCombiPresetId,
                   "unchanged automated combi preset remains stable after send") && ok;
    ok = checkPass(midi.getNumEvents() == 0,
                   "unchanged automated combi preset does not repeatedly emit MIDI") && ok;

    ok = checkPass(setIntParameterValueById(processor, "combiPreset", secondCombiPresetId),
                   "automation send behavior can set second combi preset by parameter id") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getCombiPresetId() == secondCombiPresetId,
                   "automation send behavior syncs second combi preset into state") && ok;
    ok = checkPass(midi.getNumEvents() == firstSendEventCount,
                   "second automation-changing combi preset emits one full preset MIDI batch") && ok;

    midi.clear();

    ok = checkPass(setBoolParameterValueById(processor, "sendOnPresetChange", false),
                   "automation send behavior can disable send-on-preset-change by parameter id") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(! processor.getSendOnPresetChange(),
                   "automation send behavior syncs disabled send-on-preset-change before disabled preset automation") && ok;
    ok = checkPass(midi.getNumEvents() == 0,
                   "disabling send-on-preset-change alone does not emit preset MIDI") && ok;

    midi.clear();

    ok = checkPass(setIntParameterValueById(processor, "combiPreset", disabledCombiPresetId),
                   "automation send behavior can set combi preset while send-on-preset-change is disabled") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getCombiPresetId() == disabledCombiPresetId,
                   "disabled automation-changing combi preset still syncs into state") && ok;
    ok = checkPass(midi.getNumEvents() == 0,
                   "automation-changing combi preset does not emit MIDI when send-on-preset-change is disabled") && ok;

    return ok;
}
bool verifySectionAutomationTriggeredSendRequestBehavior()
{
    bool ok = true;

    constexpr int firstStringsPresetId = 13;
    constexpr int secondStringsPresetId = 12;
    constexpr int disabledStringsPresetId = 11;

    OrchConductorAudioProcessor processor;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;

    ok = checkPass(processor.getSendOnPresetChange() == false,
                   "section automation send behavior starts with send-on-preset-change disabled") && ok;

    ok = checkPass(setBoolParameterValueById(processor, "sendOnPresetChange", true),
                   "section automation send behavior can enable send-on-preset-change by parameter id") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getSendOnPresetChange(),
                   "section automation send behavior syncs enabled send-on-preset-change before section automation") && ok;
    ok = checkPass(midi.getNumEvents() == 0,
                   "enabling send-on-preset-change alone emits no section MIDI") && ok;

    midi.clear();

    ok = checkPass(setIntParameterValueById(processor, "stringsPreset", firstStringsPresetId),
                   "section automation send behavior can set first strings preset by parameter id") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getSectionPresetId(OrchConductorAudioProcessor::Section::strings) == firstStringsPresetId,
                   "section automation send behavior syncs first strings preset into state") && ok;
    ok = checkPass(midi.getNumEvents() > 0,
                   "automation-changing strings preset emits MIDI when send-on-preset-change is enabled") && ok;

    const int firstSendEventCount = midi.getNumEvents();
    midi.clear();

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getSectionPresetId(OrchConductorAudioProcessor::Section::strings) == firstStringsPresetId,
                   "unchanged automated strings preset remains stable after send") && ok;
    ok = checkPass(midi.getNumEvents() == 0,
                   "unchanged automated strings preset does not repeatedly emit MIDI") && ok;

    ok = checkPass(setIntParameterValueById(processor, "stringsPreset", secondStringsPresetId),
                   "section automation send behavior can set second strings preset by parameter id") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getSectionPresetId(OrchConductorAudioProcessor::Section::strings) == secondStringsPresetId,
                   "section automation send behavior syncs second strings preset into state") && ok;
    ok = checkPass(midi.getNumEvents() == firstSendEventCount,
                   "second automation-changing strings preset emits one full preset MIDI batch") && ok;

    midi.clear();

    ok = checkPass(setBoolParameterValueById(processor, "sendOnPresetChange", false),
                   "section automation send behavior can disable send-on-preset-change by parameter id") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(! processor.getSendOnPresetChange(),
                   "section automation send behavior syncs disabled send-on-preset-change before disabled section automation") && ok;
    ok = checkPass(midi.getNumEvents() == 0,
                   "disabling send-on-preset-change alone emits no section MIDI") && ok;

    midi.clear();

    ok = checkPass(setIntParameterValueById(processor, "stringsPreset", disabledStringsPresetId),
                   "section automation send behavior can set strings preset while send-on-preset-change is disabled") && ok;

    processor.processBlock(buffer, midi);

    ok = checkPass(processor.getSectionPresetId(OrchConductorAudioProcessor::Section::strings) == disabledStringsPresetId,
                   "disabled automation-changing strings preset still syncs into state") && ok;
    ok = checkPass(midi.getNumEvents() == 0,
                   "automation-changing strings preset does not emit MIDI when send-on-preset-change is disabled") && ok;

    return ok;
}

// Exercises the "Import Lane Library JSON" path end to end: export the built-in
// library as a template, append a 7th narrative lane to it, import it, and
// confirm the extra lane is live while hardcoded combi CC values are untouched.
bool verifyNarrativeLibraryImportRoundTrip(OrchConductorAudioProcessor& processor)
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    bool ok = true;

    // (The processor may already carry a user NarrativeLibrary.json - the
    // template it exports below always has just the 6 built-in lanes.)
    const auto libraryFile = processor.getNarrativeLibraryFile();
    const bool hadExistingLibrary = libraryFile.existsAsFile();
    const juce::String existingLibraryBackup = hadExistingLibrary ? libraryFile.loadFileAsString() : juce::String{};

    const auto templateFile = juce::File::createTempFile(".json");
    ok = checkPass(processor.exportNarrativeLibraryTemplateToFile(templateFile),
                   "export lane library template to a file") && ok;

    auto parsed = juce::JSON::parse(templateFile.loadFileAsString());
    auto* root = parsed.getDynamicObject();
    ok = checkPass(root != nullptr, "exported template parses as a JSON object") && ok;

    if (root != nullptr)
    {
        auto lanesVar = root->getProperty("narrativeLanes");
        auto* lanes = lanesVar.getArray();
        ok = checkPass(lanes != nullptr && lanes->size() == 6, "exported template has the 6 built-in lanes") && ok;

        if (lanes != nullptr)
        {
            juce::DynamicObject::Ptr pointA = new juce::DynamicObject();
            pointA->setProperty("position", 0.0);
            pointA->setProperty("combiId", 1);
            pointA->setProperty("pitchFieldIndex", 0);

            juce::DynamicObject::Ptr pointB = new juce::DynamicObject();
            pointB->setProperty("position", 1.0);
            pointB->setProperty("combiId", 2);
            pointB->setProperty("pitchFieldIndex", 0);

            juce::Array<juce::var> points;
            points.add(juce::var(pointA.get()));
            points.add(juce::var(pointB.get()));

            juce::DynamicObject::Ptr newLane = new juce::DynamicObject();
            newLane->setProperty("id", "test_lane");
            newLane->setProperty("name", "Test Lane");
            newLane->setProperty("description", "Round-trip import test lane.");
            newLane->setProperty("points", points);

            lanes->add(juce::var(newLane.get()));
        }

        const auto importFile = juce::File::createTempFile(".json");
        importFile.replaceWithText(juce::JSON::toString(parsed, true));

        const bool imported = processor.importNarrativeLibraryFromFile(importFile);
        ok = checkPass(imported, "importNarrativeLibraryFromFile accepts the 7-lane library") && ok;
        ok = checkEquals(processor.getNarrativeLaneCount(), 7, "narrative lane count after import") && ok;
        ok = checkPass(processor.getNarrativeLaneLabel(6) == "Test Lane", "7th lane label is the imported one") && ok;
        ok = checkPass(processor.isNarrativeLibraryExternal(), "isNarrativeLibraryExternal() true after import") && ok;

        // Hardcoded combi CC values must be untouched by a lane-library import.
        processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::utilityFullOrchestra));
        const auto firstWoodwind = processor.getOutputRow(0);
        ok = checkEquals(firstWoodwind.value, 127,
                         "Full Orchestra still emits hardcoded 127 after a lane-library import") && ok;

        importFile.deleteFile();
    }

    templateFile.deleteFile();

    // Restore the user's real library file so the automated test leaves no trace.
    if (hadExistingLibrary)
        libraryFile.replaceWithText(existingLibraryBackup);
    else
        libraryFile.deleteFile();

    return ok;
#else
    juce::ignoreUnused(processor);
    return true;
#endif
}

bool verifyNarrativeLaneMaker(OrchConductorAudioProcessor& processor)
{
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    bool ok = true;

    ok = checkEquals(OrchConductorAudioProcessor::getNarrativeArcShapeNames().size(), 10, "arc shape count") && ok;

    using Shape = OrchConductorAudioProcessor::NarrativeArcShape;

    for (const auto shape : { Shape::organicBuild, Shape::archRiseFall, Shape::longFade,
                              Shape::heroicJourney, Shape::suspenseRelease, Shape::pastoralPlateau })
    {
        const auto a = processor.generateNarrativeLane(shape, 8, 0.4f, 4242);
        const auto b = processor.generateNarrativeLane(shape, 8, 0.4f, 4242);

        ok = checkEquals(static_cast<int>(a.size()), 8, "generated lane length") && ok;

        bool deterministic = (a.size() == b.size()), ascending = true, validCombis = true;
        double last = -1.0;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (i < b.size() && (a[i].combiId != b[i].combiId || a[i].position != b[i].position)) deterministic = false;
            if (a[i].position < last - 1e-9) ascending = false;
            last = a[i].position;
            if (a[i].combiId < 1 || a[i].combiId > processor.getMaxCombiPresetId()) validCombis = false;
        }
        ok = checkPass(deterministic, "generated lane is deterministic for a seed") && ok;
        ok = checkPass(ascending, "generated lane positions are ascending") && ok;
        ok = checkPass(validCombis, "generated lane combi ids are valid") && ok;
        ok = checkPass(std::abs(a.front().position) < 1e-6 && std::abs(a.back().position - 1.0) < 1e-6,
                       "generated lane spans 0..1") && ok;

        // Every generated stop carries an OrchGate response mode (1..127) and
        // amount (0..127); the mode only advances when the combi moves (a held
        // combi keeps the same articulation attitude).
        bool grValid = true, grHeldWhereCombiHeld = true;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (a[i].gateResponseMode < 1 || a[i].gateResponseMode > 127
                || a[i].gateResponseAmount < 0 || a[i].gateResponseAmount > 127)
                grValid = false;
            if (i > 0 && a[i].combiId == a[i - 1].combiId
                && a[i].gateResponseMode != a[i - 1].gateResponseMode)
                grHeldWhereCombiHeld = false;
        }
        ok = checkPass(grValid, "generated lane gate-response values are in range") && ok;
        ok = checkPass(grHeldWhereCombiHeld, "generated lane holds the response mode across a held combi") && ok;
    }

    // Low restlessness -> more repeated adjacent combis than high restlessness.
    auto repeats = [] (const std::vector<OrchConductorAudioProcessor::NarrativeLanePointEdit>& v)
    {
        int r = 0;
        for (size_t i = 1; i < v.size(); ++i) if (v[i].combiId == v[i - 1].combiId) ++r;
        return r;
    };
    int lowTotal = 0, highTotal = 0;
    for (juce::int64 s = 1; s <= 20; ++s)
    {
        lowTotal  += repeats(processor.generateNarrativeLane(Shape::organicBuild, 10, 0.05f, s));
        highTotal += repeats(processor.generateNarrativeLane(Shape::organicBuild, 10, 0.95f, s));
    }
    ok = checkPass(lowTotal > highTotal, "low restlessness repeats combis more than high restlessness") && ok;

    // Save / load / delete round-trip against the real library file (restored after).
    const auto libFile = processor.getNarrativeLibraryFile();
    const bool hadLib = libFile.existsAsFile();
    const juce::String libBackup = hadLib ? libFile.loadFileAsString() : juce::String{};

    const auto lane = processor.generateNarrativeLane(Shape::archRiseFall, 5, 0.4f, 77);

    ok = checkPass(processor.saveNarrativeLane("qa_test_lane", "QA Test Lane", "", lane),
                   "saveNarrativeLane succeeds") && ok;
    const int countWithLane = processor.getNarrativeLaneCount();

    // Find it and check the stops match.
    int idx = -1;
    for (int i = 0; i < countWithLane; ++i)
        if (processor.getNarrativeLaneId(i) == "qa_test_lane") idx = i;
    ok = checkPass(idx >= 0, "saved lane is present") && ok;

    if (idx >= 0)
    {
        ok = checkEquals(processor.getNarrativeLanePointCount(idx), 5, "saved lane stop count") && ok;
        bool combisMatch = true, gateResponseMatch = true;
        for (int i = 0; i < 5; ++i)
        {
            if (processor.getNarrativeLanePointCombiId(idx, i) != lane[(size_t) i].combiId) combisMatch = false;
            if (processor.getNarrativeLanePointGateResponseModeAt(idx, i) != lane[(size_t) i].gateResponseMode
                || processor.getNarrativeLanePointGateResponseAmountAt(idx, i) != lane[(size_t) i].gateResponseAmount)
                gateResponseMatch = false;
        }
        ok = checkPass(combisMatch, "saved lane combi ids round-trip") && ok;
        ok = checkPass(gateResponseMatch, "saved lane gate-response values round-trip") && ok;
    }

    ok = checkPass(processor.deleteNarrativeLane("qa_test_lane"), "deleteNarrativeLane succeeds") && ok;
    ok = checkEquals(processor.getNarrativeLaneCount(), countWithLane - 1, "lane count drops by one after delete") && ok;
    bool stillThere = false;
    for (int i = 0; i < processor.getNarrativeLaneCount(); ++i)
        if (processor.getNarrativeLaneId(i) == "qa_test_lane") stillThere = true;
    ok = checkPass(! stillThere, "deleted lane is gone") && ok;

    // A lane whose stops reference a USER combi must resolve to it (the
    // resolver used to reject any id >= the catalog's factory combi count).
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::strings,
                                 static_cast<int>(OrchConductorAudioProcessor::Preset::fullStrings));
    const int userCombiId = processor.createUserCombiPresetFromCurrentSections("QA Lane User Combi");
    ok = checkPass(userCombiId >= 29, "a user combi exists for the lane test") && ok;

    std::vector<OrchConductorAudioProcessor::NarrativeLanePointEdit> userLane;
    { OrchConductorAudioProcessor::NarrativeLanePointEdit p; p.position = 0.0; p.combiId = 1;           userLane.push_back(p); }
    { OrchConductorAudioProcessor::NarrativeLanePointEdit p; p.position = 1.0; p.combiId = userCombiId; userLane.push_back(p); }

    ok = checkPass(processor.saveNarrativeLane("qa_user_lane", "QA User Lane", "", userLane),
                   "saveNarrativeLane with a user-combi stop succeeds") && ok;

    int userLaneIdx = -1;
    for (int i = 0; i < processor.getNarrativeLaneCount(); ++i)
        if (processor.getNarrativeLaneId(i) == "qa_user_lane") userLaneIdx = i;

    if (userLaneIdx >= 0)
    {
        processor.setAuthorityMode(OrchConductorAudioProcessor::AuthorityMode::narrativeScan);
        processor.setNarrativeLaneIndex(userLaneIdx);
        processor.setNarrativePosition(1.0);
        processor.requestNarrativeReresolve();
        juce::AudioBuffer<float> buf(2, 64); juce::MidiBuffer mid; processor.processBlock(buf, mid);
        ok = checkEquals(processor.getResolvedNarrativeCombiId(), userCombiId,
                         "narrative lane resolves to a user combi at the end stop") && ok;
        processor.setAuthorityMode(OrchConductorAudioProcessor::AuthorityMode::manualSections);
    }
    processor.deleteNarrativeLane("qa_user_lane");

    if (hadLib) libFile.replaceWithText(libBackup);
    else        libFile.deleteFile();

    return ok;
#else
    juce::ignoreUnused(processor);
    return true;
#endif
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
    std::cout << "[INFO] Authority trial diagnostic: " << processor.getRuntimeCatalogAuthorityTrialDiagnostic() << std::endl;

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
    ok = verifyProcessorRuntimeCatalogAuthorityTrialDiagnostic(processor) && ok;
    ok = verifyHardcodedBehaviorStillAvailable(processor) && ok;
    ok = verifyProcessorStatePersistenceWithRuntimeCatalogAuthority() && ok;
    ok = verifySendOnPresetChangeAutomationPersistenceRoundTrip() && ok;
    ok = verifyPresetAutomationPersistenceRoundTrip() && ok;
    ok = verifyAutomationTriggeredSendRequestBehavior() && ok;
    ok = verifySectionAutomationTriggeredSendRequestBehavior() && ok;
    ok = verifyNarrativeLibraryImportRoundTrip(processor) && ok;
    ok = verifyNarrativeLaneMaker(processor) && ok;

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
    std::cout << "[PASS] Processor runtime catalog authority trial diagnostic remains passive." << std::endl;
    std::cout << "[PASS] Phase 5G runtime catalog authority trial diagnostic satisfied." << std::endl;
    std::cout << "[PASS] Phase 6F runtime catalog persistence restore coverage satisfied." << std::endl;
    std::cout << "[PASS] Phase 6G send-on-preset-change automation persistence integration satisfied." << std::endl;
    std::cout << "[PASS] Phase 6H preset automation persistence integration satisfied." << std::endl;
    std::cout << "[PASS] Phase 6I automation-triggered send request behavior satisfied." << std::endl;
    std::cout << "[PASS] Phase 6J section automation-triggered send request behavior satisfied." << std::endl;

    return 0;
}

