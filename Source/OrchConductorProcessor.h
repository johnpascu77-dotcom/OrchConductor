#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>
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

    // Which source drives the CC20-54 output. Phase 10F.5 adds the third mode;
    // the manual-vs-combi split previously lived only implicitly in
    // isCombiModeActive(). Not yet consumed in the send path (increment 1).
    enum class AuthorityMode
    {
        manualSections = 0,
        combiPreset,
        narrativeScan
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

    bool getSectionPresetIdsForCombiPreset (int combiPresetId,
                                        int& woodwinds,
                                        int& brass,
                                        int& percussion,
                                        int& strings) const;

    int getSectionPresetId (Section section) const;

    juce::String createUserCombiNameFromCurrentSections() const;
    // Captures the 4 section preset ids + the current manual Harp/Piano values
    // (getManualHarpValue / getManualPianoValue) into a new user combi.
    int createUserCombiPresetFromCurrentSections (const juce::String& name);

    // Manual Sections mode Harp (CC49) / Piano (CC55). -1 = Off. Setting one
    // requests a send when Send-on-Preset-Change is on, same as a section
    // preset change.
    void setManualHarpValue (int value);
    void setManualPianoValue (int value);
    int getManualHarpValue() const;
    int getManualPianoValue() const;
    bool deleteUserCombiPreset (int presetId);

    juce::File getUserCombiLibraryFile() const;
    void loadUserCombiPresetsFromUserLibrary();
    bool saveUserCombiPresetsToUserLibrary() const;
    bool importUserCombiPresetsFromJson (const juce::String& jsonText);

    juce::String exportUserCombiPresetsToJson() const;
    bool writeUserCombiPresetsJsonToFile (const juce::File& file) const;

    void setSectionPresetId (Section section, int presetId);
    void setSectionPresetIdFromUI (Section section, int presetId);

    void setPreset (Preset newPreset);
    Preset getPreset() const;

    // Phase 10F.5 narrative-scan authority state. Increment 1: parameters +
    // persistence + sync only. No resolver call, no send behaviour yet.
    AuthorityMode getAuthorityMode() const;
    void setAuthorityMode (AuthorityMode mode);

    int getNarrativeLaneIndex() const;
    void setNarrativeLaneIndex (int laneIndex);

    double getNarrativePosition() const;
    void setNarrativePosition (double position);

    void requestSendPreset();
    void requestSendAllOff();

    bool consumeSendPresetRequest();
    bool consumeSendAllOffRequest();

    void setSendOnPresetChange (bool shouldSend);
    void setSendOnPresetChangeFromUI (bool shouldSend);
    bool getSendOnPresetChange() const;

    // What processBlock does with the input MIDI stream after reading the bridge
    // CCs (102-104):
    //   Off        - drop everything; output only OrchConductor's own CC20-54.
    //   ControlCcs - forward controller events with CC >= 105 (MC's field-mask
    //                CC110-121 and any future high control CCs), drop the rest.
    //                102-104 are OC's own bridge input and stay consumed. This
    //                makes OC the single control-CC relay for the downstream
    //                tracks while still blocking the MPL collision zone (20-64).
    //   All        - forward the whole stream (inline / debugging use).
    // Default: ControlCcs. OrchConductor is a control-path node; forwarding the
    // MPL note-transform CCs would leak e.g. MPL Rate on CC23 onto the OrchGates.
    enum class InputPassthroughMode
    {
        off = 0,
        controlCcs,
        all
    };

    InputPassthroughMode getInputPassthroughMode() const;
    void setInputPassthroughMode (InputPassthroughMode mode);
    void setInputPassthroughModeFromUI (InputPassthroughMode mode);

    juce::String getPresetName() const;
    juce::String getCombiPresetName() const;
    juce::String getCombiPresetLabel (int presetId) const;
    bool getCombiPresetNarrativeMetadata (int presetId,
                                          orchconductor::NarrativeMetadata& metadata) const;
    juce::String getSectionPresetLabel (Section section, int presetId) const;
    int getMaxCombiPresetId() const;
    bool isUserCombiPresetId (int presetId) const;
    int getNextAvailableUserCombiPresetId() const;
    int getMaxSectionPresetId (Section section) const;
    bool isCombiModeActive() const;

    // True when Narrative Scan mode is selected and has resolved a valid combi.
    bool isNarrativeScanDriving() const;
    // The combi id that currently drives output: the narrative-resolved one
    // while Narrative Scan is driving, otherwise the selected combiPresetId.
    int getEffectiveCombiPresetId() const;
    // isCombiModeActive() OR narrative scan is driving.
    bool isEffectiveCombiModeActive() const;
    int getResolvedNarrativeCombiId() const;
    int getResolvedNarrativeLanePointIndex() const;
    int getLastSentFieldSelectIndex() const;

    // Runtime-catalog narrative lane metadata for the editor.
    int getNarrativeLaneCount() const;
    juce::String getNarrativeLaneLabel (int laneIndex) const;
    juce::String getNarrativeLanePointLabel (int laneIndex, int pointIndex) const;

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

    // CC49 (Harp) and CC55 (Piano): only reachable via a user combi's
    // harpValue/pianoValue override (see UserCombiPreset). 0 outside combi
    // mode or when the active combi doesn't override them.
    int getHarpCcValue() const;
    int getPianoCcValue() const;

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

    // Narrative-lane library JSON import/export (editor "Import/Export Lane
    // Library JSON" buttons). Import validates the file, adopts its narrative
    // lanes live (no rebuild), and copies it to getNarrativeLibraryFile() so
    // it auto-loads next launch. Export writes the current built-in factory
    // library JSON as an editable starting template. Both are no-ops that
    // return false when ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS is off.
    bool importNarrativeLibraryFromFile (const juce::File& file);
    bool exportNarrativeLibraryTemplateToFile (const juce::File& file) const;
    juce::String getNarrativeLibrarySourceStatus() const;
    bool isNarrativeLibraryExternal() const;

    // The auto-loaded / import-target narrative-lane library JSON:
    // <userAppData>/OrchConductor/NarrativeLibrary.json. Mirrors how
    // getUserCombiLibraryFile() works for user combis.
    juce::File getNarrativeLibraryFile() const;
private:
    static juce::String getRuntimeCatalogSectionId (Section section);

    bool tryGetRuntimeSectionPresetValueForCc (Section section,
                                               int presetId,
                                               int ccNumber,
                                               int& value) const;

    bool tryGetRuntimeCombiPresetValueForCc (int presetId,
                                             int ccNumber,
                                             int& value) const;

    struct UserCombiPreset
    {
        juce::String name;

        int woodwindsPresetId = 0;
        int brassPresetId = 0;
        int percussionPresetId = 0;
        int stringsPresetId = 0;

        // Harp (CC49) and Piano (CC55) don't belong to any of the 4 section
        // families above, so they can't ride along via a section preset id.
        // -1 means "not overridden" (sends 0); >= 0 is the literal CC value
        // to send when this combi is active. No manual/slider UI exists for
        // these two - the only way to set them is by hand-editing the
        // exported UserCombiPresets.json and re-importing it.
        int harpValue = -1;
        int pianoValue = -1;

        // Arbitrary per-CC overrides - any CC, any value, freely authored.
        // Takes precedence over everything else below (including
        // harpValue/pianoValue and the 4 section-preset ids) for whichever
        // CCs it lists; every CC it doesn't mention still resolves via the
        // section-preset composition as usual. This is what makes a combi
        // able to express something finer than "pick a whole named section
        // preset per family" - e.g. one string entering at 40 rather than
        // full value, for a graduated accumulation/transition/fade-out.
        std::vector<orchconductor::PresetValue> explicitCcValues;

        orchconductor::NarrativeMetadata metadata;
};

    std::map<int, UserCombiPreset> userCombiPresets;
    static constexpr int minCombiPresetId = 0;
    static constexpr int maxFactoryCombiPresetId = static_cast<int> (CombiPreset::soloEnglishHornLament);
    static constexpr int maxCombiPresetParameterId = 127;
    static constexpr int firstUserCombiPresetId = maxFactoryCombiPresetId + 1;

    static constexpr int minSectionPresetId = 0;

    // Phase 1H expanded section dropdown limits.
    static constexpr int maxWoodwindsPresetId = 19;
    static constexpr int maxBrassPresetId = 16;
    // Was 8 - stale, undercounted presets 9-11 ("High/Middle Orchestra
    // Percussion", "Shimmer Percussion") already present in the value
    // switches but unreachable via this parameter's own range. Fixed here
    // while extending for the 7 new unpitched percussion instruments'
    // presets (12-20) - see getPercussionPresetValueForIndex.
    static constexpr int maxPercussionPresetId = 20;

    static constexpr int minStringsPresetId = 0;
    static constexpr int maxStringsPresetId = 13;

    // Narrative lane parameter range. Kept generous; the actual usable count
    // comes from the runtime catalog and is clamped at resolve time.
    static constexpr int minNarrativeLaneParameterId = 0;
    static constexpr int maxNarrativeLaneParameterId = 15;

    int combiPresetId { static_cast<int> (CombiPreset::manualSections) };

    int woodwindsPresetId { 0 };
    int brassPresetId { 0 };
    int percussionPresetId { 0 };
    int stringsPresetId { static_cast<int> (Preset::allOff) };

    // Manual Sections mode Harp (CC49) / Piano (CC55) values. -1 = Off (send 0).
    // They have no section-preset table (harp/piano aren't part of any of the
    // 4 families' CC lists), so they are their own tiny manual control -
    // driven by the editor's Harp/Piano sliders, sent in Manual Sections mode,
    // and captured into a user combi's harpValue/pianoValue by "Save Current
    // Sections as Combi". Inert (output-wise) in Combi / Narrative Scan mode,
    // where the combi / lane point drives CC49/CC55 instead.
    int manualHarpValue { -1 };
    int manualPianoValue { -1 };

    AuthorityMode authorityMode { AuthorityMode::manualSections };
    int narrativeLaneIndex { 0 };
    double narrativePosition { 0.0 };
    int lastResolvedNarrativePointIndex { -1 };
    int lastResolvedNarrativeCombiId { -1 };

    // The resolved lane point's Harp (CC49) / Piano (CC55) overrides, -1 when
    // the point doesn't set them. Applied on top of the resolved combi's own
    // harp/piano value while Narrative Scan is driving - the only way a lane
    // can bring the Harp or Piano in (no factory combi touches CC49/CC55).
    int lastResolvedNarrativeHarpValue { -1 };
    int lastResolvedNarrativePianoValue { -1 };

    // Field-select CC (OrchNoteFilter pitch-class field, see NarrativeLanePointDefinition
    // ::pitchFieldIndex). Staged when the resolved lane point changes, emitted
    // next processBlock, suppressed when the field index is unchanged.
    int pendingFieldSelectIndex { -1 };
    int lastSentFieldSelectIndex { -1 };
    // OrchNoteFilter's field-preset list length minus one (indices 0..N). Keep in
    // sync with OrchNoteFilter's append-only preset list.
    static constexpr int maxPitchFieldIndex = 14;

    bool sendPresetRequested { false };
    bool explicitSendPresetRequested { false }; // set only by requestSendPreset(), not by narrative resolve
    bool sendAllOffRequested { false };
    bool sendOnPresetChange { false };

    // Live-rig bug (2026-09-09): a "Send Preset" click made while the
    // transport is stopped can queue the CC dump but the new percussion
    // pipeline (OrchConductor -> OrchPercMapper Arbiter -> per-instrument
    // OrchGate) sometimes never actually receives it - Bitwig appears to
    // defer running that chain's processBlock until the engine is truly
    // live, which only playback guarantees. The older direct sections
    // (woodwinds/brass/strings/harp/piano) have one fewer hop and didn't
    // show the symptom. Rather than depend on exact host timing, force one
    // authoritative full resend the moment playback actually starts, same
    // spirit as requestSendPreset() - so every downstream plugin, however
    // late it woke up, gets a guaranteed-fresh state within the same block
    // that's certainly being processed.
    bool wasHostPlaying { false };
    InputPassthroughMode inputPassthroughMode { InputPassthroughMode::controlCcs };
    static constexpr int controlCcPassthroughFloor = 105;

    OrchConductorRuntimePresetCatalog runtimePresetCatalog { OrchConductorRuntimePresetCatalog::createFallbackCatalog() };
    bool runtimePresetCatalogAuthorityActive { false };
    juce::String runtimePresetCatalogAuthorityStatus { "Runtime preset catalog authority is inactive." };

    // Guards runtimePresetCatalog against a torn read: the audio thread reads
    // it in updateNarrativeScanResolution() / tryGetRuntime*() while the
    // message thread can replace it whole from importNarrativeLibraryFromFile().
    // The critical sections are tiny and near-uncontended (import is a rare,
    // user-initiated action), so a SpinLock is the right tool.
    mutable juce::SpinLock runtimeCatalogLock;

    // Empty until a narrative-lane library JSON is loaded from disk (either the
    // auto-loaded copy at getNarrativeLibraryFile() on construction, or an
    // explicit "Import Lane Library JSON"). Only the narrative lanes + labels
    // of a loaded file take effect; CC values stay hardcoded-authoritative
    // unless ORCHCONDUCTOR_ENABLE_RUNTIME_CATALOG_AUTHORITY_TRIAL is also on.
    juce::File narrativeLibraryExternalFile;
    juce::String narrativeLibrarySourceStatus { "Narrative library: built-in (embedded factory catalog)." };

    // Runs the runtime-catalog validation gates against a loaded source and,
    // if it is a ready factory-shaped catalog, swaps it into
    // runtimePresetCatalog (under runtimeCatalogLock) so its narrative lanes
    // and labels are used. Sets all the diagnostic members. Returns true when
    // the catalog was adopted. Also called from the constructor for the
    // embedded source.
    bool runRuntimeCatalogProbe (const orchconductor::RuntimePresetSourceResult& source);

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
    juce::AudioParameterChoice* inputPassthroughParameter { nullptr };
    juce::AudioParameterChoice* authorityModeParameter { nullptr };
    juce::AudioParameterInt* narrativeLaneParameter { nullptr };
    juce::AudioParameterFloat* narrativePositionParameter { nullptr };
    juce::AudioParameterInt* fieldSelectCcParameter { nullptr };
    void syncAutomatedParameters();

    // Phase 10F.5 increment 2: resolve the selected narrative lane + position
    // to a combi id and request a send when that id changes. Only called while
    // authorityMode == narrativeScan.
    void updateNarrativeScanResolution();

    // Phase 10F.5 MC bridge: map input CC102/103/104 onto narrative position /
    // lane / authority mode. Input messages are left in the buffer untouched.
    void applyNarrativeControlCcInput (const juce::MidiBuffer& midiMessages);

    int getPresetValueForIndex (int index) const;
    int getWoodwindsPresetValueForIndex (int index) const;
    int getBrassPresetValueForIndex (int index) const;
    int getPercussionPresetValueForIndex (int index) const;
    int getSectionPresetValueForCc (Section section, int presetId, int ccNumber) const;
    int getStringsPresetValueForIndex (int presetId, int index) const;
    int getWoodwindsPresetValueForIndex (int presetId, int index) const;
    int getBrassPresetValueForIndex (int presetId, int index) const;
    int getPercussionPresetValueForIndex (int presetId, int index) const;
    int getCombiPresetValueForCc (int ccNumber) const;
    int getCombiPresetValueForCc (int presetId, int ccNumber) const;

    bool isCombiSectionGateActive (int combiPresetIdToCheck, Section section) const;
    juce::String getCombiSectionActiveCcDebugString (int combiPresetIdToCheck, Section section) const;
    void debugValidateFactoryCombiSectionCoverage() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchConductorAudioProcessor)
};


