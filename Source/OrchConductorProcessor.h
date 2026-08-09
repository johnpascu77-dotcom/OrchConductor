#pragma once

#include <JuceHeader.h>
#include "OrchConductorRuntimePresetCatalog.h"

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

    enum class Section
    {
        woodwinds = 0,
        brass,
        percussion,
        strings
    };

    enum class CombiPreset
    {
        manualSections = 0,

        utilityAllOff,
        utilityFullOrchestra,
        utilityFullOrchestraNoPercussion,
        utilityChamberOrchestra,
        utilityFullStrings,
        utilityFullWoodwinds,
        utilityFullBrass,
        utilityFullWinds,
        utilityHighOrchestra,
        utilityLowOrchestra,
        utilityMiddleOrchestra,

        romanticWarmStringsHorns,
        romanticOboeStrings,
        romanticFluteViolins,
        romanticBassoonCelli,
        romanticHornChoirStrings,

        cinematicHeroicBrassStrings,
        cinematicDarkTrailerBed,
        cinematicHighWindsShimmer,
        cinematicEpicLowPulse,

        herrmannLowReeds,
        herrmannHornKnives,
        herrmannPsychoStrings,
        herrmannSuspenseWinds,

        modernistPointillistWinds,
        modernistSparseExtremes,
        shimmerSilverShimmer,
        soloEnglishHornLament
    };

    enum class Preset
    {
        allOff = 0,
        violinIOnly,
        violinIIOnly,
        violinsOnly,
        violasOnly,
        cellosOnly,
        bassesOnly,
        upperStrings,
        lowStrings,
        stringQuartet,
        violaCello,
        celloBass,
        fullStrings,
        tutti
    };

    struct OutputRow
    {
        juce::String instrumentName;
        int ccNumber;
        int value;
        int activePlayers;
        int maxPlayers;
    };

    int getCombiPresetId() const;
    void setCombiPresetId (int presetId);
    void setCombiPresetIdFromUI (int presetId);

    int getSectionPresetId (Section section) const;
    void setSectionPresetId (Section section, int presetId);
    void setSectionPresetIdFromUI (Section section, int presetId);

    void setPreset (Preset newPreset);
    Preset getPreset() const;

    void requestSendPreset();
    void requestSendAllOff();

    bool consumeSendPresetRequest();
    bool consumeSendAllOffRequest();

    void setSendOnPresetChange (bool shouldSend);
    void setSendOnPresetChangeFromUI (bool shouldSend);
    bool getSendOnPresetChange() const;

    juce::String getPresetName() const;
    juce::String getCombiPresetName() const;
    juce::String getCombiPresetLabel (int presetId) const;
    juce::String getSectionPresetLabel (Section section, int presetId) const;
    int getMaxCombiPresetId() const;
    int getMaxSectionPresetId (Section section) const;
    bool isCombiModeActive() const;

    static int getNumOutputRows();
    OutputRow getOutputRow (int index) const;
    int getTotalActivePlayers() const;

    static int getNumWoodwindsOutputRows();
    OutputRow getWoodwindsOutputRow (int index) const;

    static int getNumBrassOutputRows();
    OutputRow getBrassOutputRow (int index) const;

    static int getNumPercussionOutputRows();
    OutputRow getPercussionOutputRow (int index) const;
    int getDefaultMaxPlayersForCc (int ccNumber) const;
    int getActivePlayersForValue (int value, int maxPlayers) const;

    bool wasRuntimeJsonPresetProbeLoaded() const;
    bool doesRuntimeJsonPresetProbeRequireFallback() const;
    juce::String getRuntimeJsonPresetProbeDiagnostic() const;

    bool wasRuntimePresetCatalogAuthorityProbeReady() const;
    bool doesRuntimePresetCatalogAuthorityProbeRequireFallback() const;
    bool doesRuntimePresetCatalogAuthorityProbeHaveExpectedFactoryShape() const;
    juce::String getRuntimePresetCatalogAuthorityProbeDiagnostic() const;

    bool wasRuntimeCatalogPayloadEquivalenceProbeRun() const;
    bool didRuntimeCatalogPayloadEquivalenceProbePass() const;
    bool wasRuntimeCatalogPayloadEquivalenceProbeBlockedByFallback() const;
    juce::String getRuntimeCatalogPayloadEquivalenceProbeDiagnostic() const;

    bool wasRuntimeCatalogCoverageAuditRun() const;
    bool didRuntimeCatalogCoverageAuditPass() const;
    bool wasRuntimeCatalogCoverageAuditBlockedByFallback() const;
    juce::String getRuntimeCatalogCoverageAuditDiagnostic() const;
    bool wasRuntimeCatalogAuthorityTrialRun() const;
    bool didRuntimeCatalogAuthorityTrialPass() const;
    bool wasRuntimeCatalogAuthorityTrialBlocked() const;
    juce::String getRuntimeCatalogAuthorityTrialDiagnostic() const;

    bool isRuntimePresetCatalogAuthorityActive() const;
    juce::String getRuntimePresetCatalogAuthorityStatus() const;
private:
    static juce::String getRuntimeCatalogSectionId (Section section);

    bool tryGetRuntimeSectionPresetValueForCc (Section section,
                                               int presetId,
                                               int ccNumber,
                                               int& value) const;

    bool tryGetRuntimeCombiPresetValueForCc (int presetId,
                                             int ccNumber,
                                             int& value) const;

    static constexpr int minCombiPresetId = 0;
    static constexpr int maxFactoryCombiPresetId = static_cast<int> (CombiPreset::soloEnglishHornLament);
    static constexpr int maxCombiPresetParameterId = 127;

    static constexpr int minSectionPresetId = 0;

    // Phase 1H expanded section dropdown limits.
    static constexpr int maxWoodwindsPresetId = 19;
    static constexpr int maxBrassPresetId = 16;
    static constexpr int maxPercussionPresetId = 8;

    static constexpr int minStringsPresetId = 0;
    static constexpr int maxStringsPresetId = static_cast<int> (Preset::tutti);

    int combiPresetId { static_cast<int> (CombiPreset::manualSections) };

    int woodwindsPresetId { 0 };
    int brassPresetId { 0 };
    int percussionPresetId { 0 };
    int stringsPresetId { static_cast<int> (Preset::allOff) };

    bool sendPresetRequested { false };
    bool sendAllOffRequested { false };
    bool sendOnPresetChange { false };

    OrchConductorRuntimePresetCatalog runtimePresetCatalog { OrchConductorRuntimePresetCatalog::createFallbackCatalog() };
    bool runtimePresetCatalogAuthorityActive { false };
    juce::String runtimePresetCatalogAuthorityStatus { "Runtime preset catalog authority is inactive." };

    bool runtimeJsonPresetProbeLoaded { false };
    bool runtimeJsonPresetProbeRequiresFallback { true };
    juce::String runtimeJsonPresetProbeDiagnostic { "Runtime JSON presets are disabled by ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS." };

    bool runtimePresetCatalogAuthorityProbeReady { false };
    bool runtimePresetCatalogAuthorityProbeRequiresFallback { true };
    bool runtimePresetCatalogAuthorityProbeHasExpectedFactoryShape { false };
    juce::String runtimePresetCatalogAuthorityProbeDiagnostic { "Runtime preset catalog authority probe is inactive because runtime JSON presets are disabled." };

    bool runtimeCatalogPayloadEquivalenceProbeRun { false };
    bool runtimeCatalogPayloadEquivalenceProbePassed { false };
    bool runtimeCatalogPayloadEquivalenceProbeBlockedByFallback { true };
    juce::String runtimeCatalogPayloadEquivalenceProbeDiagnostic { "Runtime catalog payload equivalence probe is inactive because runtime JSON presets are disabled." };

    bool runtimeCatalogCoverageAuditRun { false };
    bool runtimeCatalogCoverageAuditPassed { false };
    bool runtimeCatalogCoverageAuditBlockedByFallback { true };
    juce::String runtimeCatalogCoverageAuditDiagnostic { "Runtime catalog coverage audit is inactive because runtime JSON presets are disabled." };

    bool runtimeCatalogAuthorityTrialRun{ false };
    bool runtimeCatalogAuthorityTrialPass{ false };
    bool runtimeCatalogAuthorityTrialBlocked{ true };
    juce::String runtimeCatalogAuthorityTrialDiagnostic{ "Runtime catalog authority trial is disabled by ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL." };

    juce::AudioParameterInt* combiPresetParameter { nullptr };
    juce::AudioParameterInt* woodwindsPresetParameter { nullptr };
    juce::AudioParameterInt* brassPresetParameter { nullptr };
    juce::AudioParameterInt* percussionPresetParameter { nullptr };
    juce::AudioParameterInt* stringsPresetParameter { nullptr };
    juce::AudioParameterBool* sendOnPresetChangeParameter { nullptr };
    void syncAutomatedParameters();

    int getPresetValueForIndex (int index) const;
    int getWoodwindsPresetValueForIndex (int index) const;
    int getBrassPresetValueForIndex (int index) const;
    int getPercussionPresetValueForIndex (int index) const;
    int getCombiPresetValueForCc (int ccNumber) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchConductorAudioProcessor)
};


