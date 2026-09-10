#include <JuceHeader.h>

#include "../Source/OrchConductorProcessor.h"

#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace
{

constexpr int expectedMidiChannel = 1;
// 35 original instrument CCs (20-54) + CC55 (Piano, alongside Harp CC49's
// user-combi-only override) + the 7 unpitched percussion instruments
// (56-62), all always-sent alongside Timpani/mallets.
constexpr int expectedSendCcCount = 43;

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

    ok = checkPass(captured.ccValues.count(55) == 1, context + " contains Piano CC55") && ok;

    for (int cc = 56; cc <= 62; ++cc)
        ok = checkPass(captured.ccValues.count(cc) == 1, context + " contains unpitched percussion CC" + juce::String(cc)) && ok;

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

    for (int cc = 20; cc <= 62; ++cc)
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
    ok = expectCcValue(captured, 55, 0, "manual low strings piano") && ok;

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

    ok = expectCcValue(captured, 55, 0, "manual full winds/brass piano") && ok;

    for (int cc = 56; cc <= 62; ++cc)
        ok = expectCcValue(captured, cc, 0, "manual unpitched percussion all off") && ok;

    return ok;
}

bool verifyManualUnpitchedPercussionPresets()
{
    bool ok = true;

    // "Bass Drum Only" (preset 12): only CC56 on, every other percussion
    // row (Timpani/mallets and the other 6 unpitched instruments) off.
    {
        OrchConductorAudioProcessor processor;
        processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::manualSections));
        processor.setSectionPresetId(OrchConductorAudioProcessor::Section::percussion, 12);
        processor.requestSendPreset();

        const auto captured = captureMidi(processor);

        ok = verifyStandardSendShape(captured, "manual Bass Drum Only send") && ok;
        ok = expectCcValue(captured, 56, 127, "Bass Drum Only bass drum") && ok;

        for (int cc = 43; cc <= 48; ++cc)
            ok = expectCcValue(captured, cc, 0, "Bass Drum Only mallets/timpani off") && ok;

        for (int cc = 57; cc <= 62; ++cc)
            ok = expectCcValue(captured, cc, 0, "Bass Drum Only other unpitched off") && ok;
    }

    // "Unpitched Percussion" (preset 19): all 7 unpitched instruments on,
    // Timpani/mallets untouched.
    {
        OrchConductorAudioProcessor processor;
        processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::manualSections));
        processor.setSectionPresetId(OrchConductorAudioProcessor::Section::percussion, 19);
        processor.requestSendPreset();

        const auto captured = captureMidi(processor);

        for (int cc = 56; cc <= 62; ++cc)
            ok = expectCcValue(captured, cc, 127, "Unpitched Percussion all 7 on") && ok;

        for (int cc = 43; cc <= 48; ++cc)
            ok = expectCcValue(captured, cc, 0, "Unpitched Percussion mallets/timpani off") && ok;
    }

    // "Full Percussion" (preset 20): every one of the 13 percussion rows on.
    {
        OrchConductorAudioProcessor processor;
        processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::manualSections));
        processor.setSectionPresetId(OrchConductorAudioProcessor::Section::percussion, 20);
        processor.requestSendPreset();

        const auto captured = captureMidi(processor);

        for (int cc = 43; cc <= 48; ++cc)
            ok = expectCcValue(captured, cc, 127, "Full Percussion mallets/timpani on") && ok;

        for (int cc = 56; cc <= 62; ++cc)
            ok = expectCcValue(captured, cc, 127, "Full Percussion unpitched on") && ok;
    }

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
    for (int cc = 20; cc <= 62; ++cc)
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

bool verifyUserCombiHarpPianoOverride()
{
    OrchConductorAudioProcessor processor;

    const juce::String importJson = R"JSON(
    {
        "schema": "orch_conductor_user_combi_presets",
        "version": 1,
        "combiPresets": [
            {
                "name": "Test Harp Piano Override",
                "sections": { "woodwinds": 0, "brass": 0, "percussion": 0, "strings": 0 },
                "harpValue": 100,
                "pianoValue": 90
            }
        ]
    }
    )JSON";

    bool ok = checkPass(processor.importUserCombiPresetsFromJson(importJson),
                         "user combi harp/piano override JSON imported");

    // importUserCombiPresetsFromJson *replaces* the whole user-combi map (it
    // may have pre-existing entries loaded from this machine's real
    // UserCombiPresets.json on construction), so the id our one entry landed
    // on is whatever the map's max id is right after import - not something
    // predictable from getNextAvailableUserCombiPresetId() beforehand.
    const int importedCombiId = processor.getMaxCombiPresetId();

    processor.setCombiPresetId(importedCombiId);
    processor.requestSendPreset();

    const auto captured = captureMidi(processor);

    ok = verifyStandardSendShape(captured, "user combi harp/piano override send") && ok;

    ok = expectCcValue(captured, 49, 100, "user combi harp override") && ok;
    ok = expectCcValue(captured, 55, 90, "user combi piano override") && ok;

    for (int cc = 20; cc <= 48; ++cc)
        ok = expectCcValue(captured, cc, 0, "user combi harp/piano override (all off elsewhere)") && ok;

    for (int cc = 50; cc <= 54; ++cc)
        ok = expectCcValue(captured, cc, 0, "user combi harp/piano override (all off elsewhere)") && ok;

    for (int cc = 56; cc <= 62; ++cc)
        ok = expectCcValue(captured, cc, 0, "user combi harp/piano override (all off elsewhere)") && ok;

    // A combi that doesn't set harpValue/pianoValue at all (an "unset" -1,
    // matching a plain factory combi) must still resolve both to 0.
    const juce::String noOverrideJson = R"JSON(
    {
        "schema": "orch_conductor_user_combi_presets",
        "version": 1,
        "combiPresets": [
            {
                "name": "Test No Override",
                "sections": { "woodwinds": 0, "brass": 0, "percussion": 0, "strings": 0 }
            }
        ]
    }
    )JSON";

    OrchConductorAudioProcessor processor2;

    ok = checkPass(processor2.importUserCombiPresetsFromJson(noOverrideJson),
                    "user combi no-override JSON imported") && ok;

    processor2.setCombiPresetId(processor2.getMaxCombiPresetId());
    processor2.requestSendPreset();

    const auto captured2 = captureMidi(processor2);

    ok = expectCcValue(captured2, 49, 0, "user combi no-override harp") && ok;
    ok = expectCcValue(captured2, 55, 0, "user combi no-override piano") && ok;

    return ok;
}

bool verifyUserCombiExplicitCcValues()
{
    OrchConductorAudioProcessor processor;

    // Every section is "All Off", but two explicit CC overrides are set:
    // CC46 (Marimba - not otherwise touched by any section here) at an
    // arbitrary intermediate value, and CC49 (Harp) at a value that
    // deliberately differs from harpValue, to confirm explicitCcValues
    // wins over both the section composition AND the harpValue/pianoValue
    // special case.
    const juce::String importJson = R"JSON(
    {
        "schema": "orch_conductor_user_combi_presets",
        "version": 1,
        "combiPresets": [
            {
                "name": "Test Explicit CC Values",
                "sections": { "woodwinds": 0, "brass": 0, "percussion": 0, "strings": 0 },
                "harpValue": 50,
                "values": [
                    { "cc": 46, "value": 90 },
                    { "cc": 49, "value": 77 }
                ]
            }
        ]
    }
    )JSON";

    bool ok = checkPass(processor.importUserCombiPresetsFromJson(importJson),
                         "user combi explicit CC values JSON imported");

    const int importedCombiId = processor.getMaxCombiPresetId();

    processor.setCombiPresetId(importedCombiId);
    processor.requestSendPreset();

    const auto captured = captureMidi(processor);

    ok = verifyStandardSendShape(captured, "user combi explicit CC values send") && ok;

    ok = expectCcValue(captured, 46, 90, "user combi explicit CC value (Marimba)") && ok;
    ok = expectCcValue(captured, 49, 77, "user combi explicit CC value overrides harpValue") && ok;

    for (int cc = 20; cc <= 48; ++cc)
        if (cc != 46)
            ok = expectCcValue(captured, cc, 0, "user combi explicit CC values (all off elsewhere)") && ok;

    for (int cc = 50; cc <= 62; ++cc)
        ok = expectCcValue(captured, cc, 0, "user combi explicit CC values (all off elsewhere)") && ok;

    return ok;
}

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
CapturedMidi captureMidiWithInput (OrchConductorAudioProcessor& processor,
                                   int inputCc,
                                   int inputValue)
{
    juce::AudioBuffer<float> buffer (2, 64);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::controllerEvent (1, inputCc, inputValue), 0);

    buffer.clear();
    processor.processBlock (buffer, midi);

    CapturedMidi captured;
    captured.eventCount = midi.getNumEvents();

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isController())
        {
            captured.channels.push_back (message.getChannel());
            captured.ccValues[message.getControllerNumber()] = message.getControllerValue();
        }
    }

    return captured;
}

bool verifyNarrativeControlCcInput()
{
    OrchConductorAudioProcessor processor;

    if (processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback())
    {
        std::cout << "[SKIP] narrative control CC input: runtime catalog not authoritative" << std::endl;
        return true;
    }

    bool ok = true;

    // CC104 high -> Narrative Scan authority mode.
    captureMidiWithInput (processor, 104, 127);
    ok = checkEquals (static_cast<int> (processor.getAuthorityMode()),
                      static_cast<int> (OrchConductorAudioProcessor::AuthorityMode::narrativeScan),
                      "CC104=127 selects Narrative Scan authority") && ok;

    // CC103 -> narrative lane index.
    captureMidiWithInput (processor, 103, 1);
    ok = checkEquals (processor.getNarrativeLaneIndex(), 1, "CC103=1 selects narrative lane 1") && ok;

    // CC102 high -> narrative position ~1.0, resolves a combi and sends it.
    // The input CC102 itself is consumed (passthrough defaults off), so the
    // output is exactly the 35-CC combi payload.
    const auto captured = captureMidiWithInput (processor, 102, 127);
    ok = checkPass (processor.getNarrativePosition() > 0.99,
                    "CC102=127 drives narrative position to ~1.0") && ok;
    ok = checkPass (captured.ccValues.count (102) == 0, "input CC102 is consumed, not forwarded") && ok;
    ok = checkEquals (captured.eventCount, expectedSendCcCount,
                      "CC102 position change triggered a full combi send") && ok;
    ok = checkPass (processor.getResolvedNarrativeCombiId() >= 0,
                    "CC-driven narrative scan resolved a combi") && ok;

    // CC104 low -> back to Manual Sections.
    captureMidiWithInput (processor, 104, 0);
    ok = checkEquals (static_cast<int> (processor.getAuthorityMode()),
                      static_cast<int> (OrchConductorAudioProcessor::AuthorityMode::manualSections),
                      "CC104=0 selects Manual Sections authority") && ok;

    return ok;
}

bool verifyInputPassthroughModes()
{
    using Mode = OrchConductorAudioProcessor::InputPassthroughMode;

    bool ok = true;

    // Default is "Control CCs (>= 100)": a low CC (MPL Rate on 23) is dropped,
    // a high CC (MC field mask on 110) is forwarded.
    {
        OrchConductorAudioProcessor processor;
        ok = checkEquals (static_cast<int> (processor.getInputPassthroughMode()),
                          static_cast<int> (Mode::controlCcs), "default passthrough mode is Control CCs") && ok;

        const auto low = captureMidiWithInput (processor, 23, 100);
        ok = checkPass (low.ccValues.count (23) == 0, "Control CCs: input CC23 is dropped") && ok;

        const auto high = captureMidiWithInput (processor, 110, 127);
        ok = checkPass (high.ccValues.count (110) == 1, "Control CCs: input CC110 is forwarded") && ok;
    }

    // Off: everything dropped.
    {
        OrchConductorAudioProcessor processor;
        processor.setInputPassthroughMode (Mode::off);
        const auto captured = captureMidiWithInput (processor, 110, 127);
        ok = checkPass (captured.ccValues.count (110) == 0, "Off: input CC110 is dropped") && ok;
        ok = checkEquals (captured.eventCount, 0, "Off: no output without a send request") && ok;
    }

    // All: low CC forwarded.
    {
        OrchConductorAudioProcessor processor;
        processor.setInputPassthroughMode (Mode::all);
        const auto captured = captureMidiWithInput (processor, 23, 100);
        ok = checkPass (captured.ccValues.count (23) == 1, "All: input CC23 is forwarded") && ok;
    }

    // Bridge CCs (102-104) are read regardless of mode (consumed before the filter).
    {
        OrchConductorAudioProcessor processor; // default Control CCs
        captureMidiWithInput (processor, 104, 127);
        ok = checkEquals (static_cast<int> (processor.getAuthorityMode()),
                          static_cast<int> (OrchConductorAudioProcessor::AuthorityMode::narrativeScan),
                          "CC104 still read under Control CCs mode") && ok;
    }

    return ok;
}

bool verifyNarrativeScanDrivesCombiSend()
{
    OrchConductorAudioProcessor processor;

    // Narrative lanes only exist when the runtime catalog loaded and became
    // authoritative. If it did not, skip rather than fail - this mirrors the
    // resolver's own safe-degradation behaviour.
    if (processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback())
    {
        std::cout << "[SKIP] narrative scan: runtime catalog not authoritative" << std::endl;
        return true;
    }

    processor.setAuthorityMode(OrchConductorAudioProcessor::AuthorityMode::narrativeScan);
    processor.setNarrativeLaneIndex(0);   // organic_build
    processor.setNarrativePosition(0.0);  // -> lane point 0 -> combi 28 (Solo English Horn Lament)

    const auto atStart = captureMidi(processor);

    bool ok = true;

    // Start: point 0 -> combi 28 + field #12 (CC105). Full combi payload plus
    // the one field-select CC.
    ok = checkEquals(atStart.eventCount, expectedSendCcCount + 1, "narrative scan start event count") && ok;
    ok = checkEquals(processor.getResolvedNarrativeCombiId(),
                     static_cast<int>(OrchConductorAudioProcessor::CombiPreset::soloEnglishHornLament),
                     "narrative scan start resolved combi id") && ok;

    for (int cc = 20; cc <= 54; ++cc)
    {
        if (cc == 25 || cc == 52 || cc == 53)
            ok = expectCcValue(atStart, cc, 127, "narrative scan start") && ok;
        else if (cc == 54)
            ok = expectCcValue(atStart, cc, 64, "narrative scan start") && ok;
        else
            ok = expectCcValue(atStart, cc, 0, "narrative scan start") && ok;
    }

    // field #12 -> round(12/14 * 127) == 109
    ok = expectCcValue(atStart, 105, 109, "narrative scan start field select") && ok;
    ok = checkEquals(processor.getLastSentFieldSelectIndex(), 12, "narrative scan start field index") && ok;

    // Same position again: resolved combi + field unchanged -> no send.
    const auto held = captureMidi(processor);
    ok = checkEquals(held.eventCount, 0, "narrative scan no send while combi/field unchanged") && ok;

    // Explicit "Send Current Presets" re-emits the field CC (for a late-joining
    // OrchNoteFilter) even though nothing resolved-changed.
    processor.requestSendPreset();
    const auto resync = captureMidi(processor);
    ok = expectCcValue(resync, 105, 109, "narrative scan re-sync re-emits field select") && ok;

    // Move to the far end of the lane -> point 5 -> combi 2 + field #0.
    processor.setNarrativePosition(1.0);
    const auto atEnd = captureMidi(processor);

    ok = checkEquals(atEnd.eventCount, expectedSendCcCount + 1, "narrative scan end event count") && ok;
    ok = checkEquals(processor.getResolvedNarrativeCombiId(),
                     static_cast<int>(OrchConductorAudioProcessor::CombiPreset::utilityFullOrchestra),
                     "narrative scan end resolved combi id") && ok;

    for (int cc = 20; cc <= 48; ++cc)
        ok = expectCcValue(atEnd, cc, 127, "narrative scan end full orchestra") && ok;

    // organic_build's final point now sets harpValue = pianoValue = 127 (the
    // only way a narrative lane brings Harp/Piano in - no factory combi does).
    ok = expectCcValue(atEnd, 49, 127, "narrative scan end lane-point harp override") && ok;
    ok = expectCcValue(atEnd, 55, 127, "narrative scan end lane-point piano override") && ok;

    for (int cc = 50; cc <= 54; ++cc)
        ok = expectCcValue(atEnd, cc, 127, "narrative scan end full strings") && ok;

    // "Full Orchestra" (the organic_build lane's end point) now includes the
    // 7 unpitched percussion instruments - CC56-62 - just like the pitched
    // percussion CC43-48 above. Added 2026-09-10 in both the hardcoded combi
    // table and the embedded JSON catalog.
    for (int cc = 56; cc <= 62; ++cc)
        ok = expectCcValue(atEnd, cc, 127, "narrative scan end full orchestra unpitched percussion") && ok;

    ok = expectCcValue(atEnd, 105, 0, "narrative scan end field select (#0)") && ok;
    ok = checkEquals(processor.getLastSentFieldSelectIndex(), 0, "narrative scan end field index") && ok;

    // Back to point 3 (position 0.55): harpValue 80, no pianoValue -> piano
    // drops back to the combi's own value (0). The harp/piano change alone
    // triggers the resend even though this is a lane-point move.
    processor.setNarrativePosition(0.55);
    const auto atMid = captureMidi(processor);
    ok = checkPass(atMid.eventCount > 0, "narrative scan mid-point move triggers a resend") && ok;
    ok = expectCcValue(atMid, 49, 80, "narrative scan mid-point harp override (80)") && ok;
    ok = expectCcValue(atMid, 55, 0, "narrative scan mid-point piano falls back to 0") && ok;

    return ok;
}
#endif

bool verifyInstrumentGridUserCombi()
{
    OrchConductorAudioProcessor processor;
    bool ok = true;

    const int slotCount = OrchConductorAudioProcessor::getInstrumentSlotCount();
    ok = checkEquals(slotCount, 43, "instrument grid slot count") && ok;

    // Every slot gets a distinct value: slot i -> (i * 3) % 128.
    std::vector<orchconductor::PresetValue> values;
    for (int i = 0; i < slotCount; ++i)
    {
        orchconductor::PresetValue v;
        v.ccNumber = processor.getInstrumentSlotCc(i);
        v.value = (i * 3) % 128;
        values.push_back(v);
    }

    const int id = processor.saveInstrumentGridAsUserCombi("Grid Test", values, -1);
    ok = checkPass(id >= 0, "saveInstrumentGridAsUserCombi returns a combi id") && ok;

    // Round-trips through getUserCombiExplicitCcValues.
    const auto stored = processor.getUserCombiExplicitCcValues(id);
    ok = checkEquals(static_cast<int>(stored.size()), slotCount, "stored explicit CC count") && ok;

    // Selecting the combi -> its output is exactly the grid.
    processor.setCombiPresetId(id);
    processor.requestSendPreset();
    const auto captured = captureMidi(processor);

    bool allMatch = true;
    for (int i = 0; i < slotCount; ++i)
    {
        const int cc = processor.getInstrumentSlotCc(i);
        const int expected = (i * 3) % 128;
        if (captured.ccValues.count(cc) == 0 || captured.ccValues.at(cc) != expected)
            allMatch = false;
    }
    ok = checkPass(allMatch, "grid combi output matches every slider value (incl. CC49/CC55)") && ok;

    // Update in place: same id, new values (all 100).
    for (auto& v : values) v.value = 100;
    const int updatedId = processor.saveInstrumentGridAsUserCombi("Grid Test", values, id);
    ok = checkEquals(updatedId, id, "update reuses the same combi id") && ok;

    processor.requestSendPreset();
    const auto captured2 = captureMidi(processor);
    ok = expectCcValue(captured2, 20, 100, "updated grid combi CC20") && ok;
    ok = expectCcValue(captured2, 49, 100, "updated grid combi CC49 (Harp)") && ok;
    ok = expectCcValue(captured2, 62, 100, "updated grid combi CC62 (Triangle)") && ok;

    return ok;
}

bool verifyRandomGridGenerator()
{
    OrchConductorAudioProcessor processor;
    bool ok = true;

    using Style = OrchConductorAudioProcessor::GridRandomStyle;
    const int slotCount = OrchConductorAudioProcessor::getInstrumentSlotCount();

    for (const auto style : { Style::balanced, Style::feature, Style::sparse, Style::tutti })
    {
        const auto a = processor.generateRandomGridCombi(style, 12345);
        const auto b = processor.generateRandomGridCombi(style, 12345);

        ok = checkEquals(static_cast<int>(a.size()), slotCount, "random grid size") && ok;

        bool deterministic = a.size() == b.size();
        for (size_t i = 0; deterministic && i < a.size(); ++i)
            deterministic = (a[i].ccNumber == b[i].ccNumber && a[i].value == b[i].value);
        ok = checkPass(deterministic, "random grid is deterministic for a seed") && ok;

        int nonZero = 0;
        bool inRange = true;
        for (const auto& pv : a)
        {
            if (pv.value < 0 || pv.value > 127) inRange = false;
            if (pv.value > 0) ++nonZero;
        }
        ok = checkPass(inRange, "random grid values are 0-127") && ok;
        ok = checkPass(nonZero > 0, "random grid is never silent") && ok;
    }

    // Tutti is denser than Sparse (averaged over several seeds).
    int tuttiTotal = 0, sparseTotal = 0;
    for (juce::int64 s = 1; s <= 12; ++s)
    {
        for (const auto& pv : processor.generateRandomGridCombi(Style::tutti, s))  tuttiTotal  += (pv.value > 0);
        for (const auto& pv : processor.generateRandomGridCombi(Style::sparse, s)) sparseTotal += (pv.value > 0);
    }
    ok = checkPass(tuttiTotal > sparseTotal, "Tutti random grid is denser than Sparse") && ok;

    // A generated grid saves and reproduces exactly.
    const auto values = processor.generateRandomGridCombi(Style::balanced, 999);
    const int id = processor.saveInstrumentGridAsUserCombi("Random", values, -1);
    ok = checkPass(id >= 0, "generated grid saves as a combi") && ok;

    processor.setCombiPresetId(id);
    processor.requestSendPreset();
    const auto captured = captureMidi(processor);

    bool match = true;
    for (const auto& pv : values)
        if (captured.ccValues.count(pv.ccNumber) == 0 || captured.ccValues.at(pv.ccNumber) != pv.value)
            match = false;
    ok = checkPass(match, "generated grid combi output matches the generated values") && ok;

    return ok;
}

bool verifyManualHarpPianoSliders()
{
    OrchConductorAudioProcessor processor;
    bool ok = true;

    // Manual Sections mode: the manual Harp/Piano values drive CC49/CC55.
    processor.setManualHarpValue(90);
    processor.setManualPianoValue(70);
    processor.requestSendPreset();

    const auto manual = captureMidi(processor);
    ok = checkEquals(manual.eventCount, expectedSendCcCount, "manual harp/piano send event count") && ok;
    ok = expectCcValue(manual, 49, 90, "manual sections harp slider drives CC49") && ok;
    ok = expectCcValue(manual, 55, 70, "manual sections piano slider drives CC55") && ok;

    // "Off" (-1) sends 0.
    processor.setManualHarpValue(-1);
    processor.requestSendPreset();
    const auto harpOff = captureMidi(processor);
    ok = expectCcValue(harpOff, 49, 0, "manual sections harp Off sends CC49 = 0") && ok;
    ok = expectCcValue(harpOff, 55, 70, "manual sections piano unchanged at 70") && ok;

    // Combi mode: a factory combi's own harp/piano (0) wins - the manual
    // sliders are inert for output.
    processor.setManualHarpValue(100);
    processor.setManualPianoValue(100);
    processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::utilityFullOrchestra));
    processor.requestSendPreset();
    const auto inCombi = captureMidi(processor);
    ok = expectCcValue(inCombi, 49, 0, "combi mode ignores manual harp slider") && ok;
    ok = expectCcValue(inCombi, 55, 0, "combi mode ignores manual piano slider") && ok;

    // State round-trip (v6) preserves the manual values.
    processor.setCombiPresetId(static_cast<int>(OrchConductorAudioProcessor::CombiPreset::manualSections));
    processor.setManualHarpValue(42);
    processor.setManualPianoValue(24);

    juce::MemoryBlock state;
    processor.getStateInformation(state);

    OrchConductorAudioProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    ok = checkEquals(restored.getManualHarpValue(), 42, "manual harp value survives state round-trip") && ok;
    ok = checkEquals(restored.getManualPianoValue(), 24, "manual piano value survives state round-trip") && ok;

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

bool verifyGateResponseManualPanel()
{
    OrchConductorAudioProcessor processor;
    bool ok = true;

    // Panel off: a normal payload send carries no CC106/107.
    processor.setSectionPresetId(OrchConductorAudioProcessor::Section::strings,
                                 static_cast<int>(OrchConductorAudioProcessor::Preset::lowStrings));
    processor.requestSendPreset();
    const auto quiet = captureMidi(processor);
    ok = checkEquals(quiet.eventCount, expectedSendCcCount, "gate response panel off: plain payload") && ok;
    ok = checkPass(quiet.ccValues.count(106) == 0, "gate response panel off: no CC106") && ok;
    ok = checkPass(quiet.ccValues.count(107) == 0, "gate response panel off: no CC107") && ok;

    // Enable the panel with a mode/amount -> next send broadcasts both.
    processor.setGateResponseManualMode(70);
    processor.setGateResponseManualAmount(110);
    processor.setGateResponseManualEnabled(true);
    const auto armed = captureMidi(processor);
    ok = checkEquals(armed.eventCount, expectedSendCcCount + 2, "gate response armed: payload + 2 bridge CCs") && ok;
    ok = expectCcValue(armed, 106, 70, "gate response armed CC106 = mode") && ok;
    ok = expectCcValue(armed, 107, 110, "gate response armed CC107 = amount") && ok;

    // Explicit "Send Current Presets" re-broadcasts (same contract as the
    // field-select CC) so a late-joining OrchGate is brought current.
    processor.requestSendPreset();
    const auto resync = captureMidi(processor);
    ok = expectCcValue(resync, 106, 70, "gate response re-sync CC106") && ok;
    ok = expectCcValue(resync, 107, 110, "gate response re-sync CC107") && ok;

    // Disable the panel -> amount 0 emitted once (every OrchGate back to knobs).
    processor.setGateResponseManualEnabled(false);
    const auto released = captureMidi(processor);
    ok = expectCcValue(released, 107, 0, "gate response disabled emits CC107 = 0") && ok;

    // ...and not again on the next send.
    processor.requestSendPreset();
    const auto after = captureMidi(processor);
    ok = checkPass(after.ccValues.count(107) == 0, "gate response disabled: CC107 = 0 not repeated") && ok;

    // Send All Off after arming neutralises the bridge.
    processor.setGateResponseManualEnabled(true);
    processor.setGateResponseManualAmount(90);
    captureMidi(processor);
    processor.requestSendAllOff();
    const auto allOff = captureMidi(processor);
    ok = expectCcValue(allOff, 107, 0, "Send All Off after arming emits CC107 = 0") && ok;

    // Shuffle enables the panel and lands on a non-zero mode.
    OrchConductorAudioProcessor shuffled;
    shuffled.shuffleGateResponseMode();
    ok = checkPass(shuffled.isGateResponseManualEnabled(), "shuffle enables the panel") && ok;
    ok = checkPass(shuffled.getGateResponseManualMode() >= 1 && shuffled.getGateResponseManualMode() <= 127,
                   "shuffle lands on a 1..127 mode") && ok;

    // State v7 round-trip.
    OrchConductorAudioProcessor src;
    src.setGateResponseManualMode(55);
    src.setGateResponseManualAmount(33);
    src.setGateResponseManualEnabled(true);
    juce::MemoryBlock state;
    src.getStateInformation(state);
    OrchConductorAudioProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    ok = checkPass(restored.isGateResponseManualEnabled(), "gate response enabled survives state round-trip") && ok;
    ok = checkEquals(restored.getGateResponseManualMode(), 55, "gate response mode survives state round-trip") && ok;
    ok = checkEquals(restored.getGateResponseManualAmount(), 33, "gate response amount survives state round-trip") && ok;

    return ok;
}

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
bool verifyGateResponseLanePoint()
{
    OrchConductorAudioProcessor processor;

    if (processor.doesRuntimePresetCatalogAuthorityProbeRequireFallback())
    {
        std::cout << "[SKIP] gate response lane point: runtime catalog not authoritative" << std::endl;
        return true;
    }

    bool ok = true;

    // Build a throwaway lane with explicit gate-response stops (the embedded
    // demo lane may be shadowed by the user's own NarrativeLibrary.json).
    const auto libFile = processor.getNarrativeLibraryFile();
    const bool hadLib = libFile.existsAsFile();
    const juce::String libBackup = hadLib ? libFile.loadFileAsString() : juce::String{};

    std::vector<OrchConductorAudioProcessor::NarrativeLanePointEdit> lane;
    { OrchConductorAudioProcessor::NarrativeLanePointEdit p; p.position = 0.0; p.combiId = 1;                       lane.push_back(p); }
    { OrchConductorAudioProcessor::NarrativeLanePointEdit p; p.position = 0.5; p.combiId = 4; p.gateResponseMode = 40; p.gateResponseAmount = 48; lane.push_back(p); }
    { OrchConductorAudioProcessor::NarrativeLanePointEdit p; p.position = 0.75; p.combiId = 4; p.gateResponseMode = 40; p.gateResponseAmount = 84; lane.push_back(p); }
    { OrchConductorAudioProcessor::NarrativeLanePointEdit p; p.position = 1.0; p.combiId = 2;                       lane.push_back(p); }

    ok = checkPass(processor.saveNarrativeLane("qa_gate_response_lane", "QA Gate Response", "", lane),
                   "saveNarrativeLane (gate response) succeeds") && ok;

    int laneIdx = -1;
    for (int i = 0; i < processor.getNarrativeLaneCount(); ++i)
        if (processor.getNarrativeLaneId(i) == "qa_gate_response_lane") laneIdx = i;

    if (laneIdx >= 0)
    {
        processor.setAuthorityMode(OrchConductorAudioProcessor::AuthorityMode::narrativeScan);
        processor.setNarrativeLaneIndex(laneIdx);

        // Point 0 (position 0) carries no gate-response -> no CC106/107.
        processor.setNarrativePosition(0.0);
        processor.requestNarrativeReresolve();
        const auto atStart = captureMidi(processor);
        ok = checkPass(atStart.ccValues.count(106) == 0, "lane point 0: no CC106") && ok;
        ok = checkPass(atStart.ccValues.count(107) == 0, "lane point 0: no CC107") && ok;

        // Point at 0.5 sets gateResponseMode 40 / gateResponseAmount 48.
        processor.setNarrativePosition(0.5);
        const auto atMid = captureMidi(processor);
        ok = expectCcValue(atMid, 106, 40, "lane point 0.5 broadcasts gateResponseMode") && ok;
        ok = expectCcValue(atMid, 107, 48, "lane point 0.5 broadcasts gateResponseAmount") && ok;

        // Point at 0.75 keeps mode 40 (held chapter) but lifts amount to 84 ->
        // only CC107 is re-sent.
        processor.setNarrativePosition(0.75);
        const auto atLate = captureMidi(processor);
        ok = checkPass(atLate.ccValues.count(106) == 0, "lane point 0.75 holds the mode (no CC106)") && ok;
        ok = expectCcValue(atLate, 107, 84, "lane point 0.75 lifts gateResponseAmount") && ok;

        // Far end (position 1) has no gate-response -> amount neutralised to 0.
        processor.setNarrativePosition(1.0);
        const auto atEnd = captureMidi(processor);
        ok = expectCcValue(atEnd, 107, 0, "lane end with no gate-response neutralises CC107") && ok;

        processor.setAuthorityMode(OrchConductorAudioProcessor::AuthorityMode::manualSections);
    }

    processor.deleteNarrativeLane("qa_gate_response_lane");

    if (hadLib) libFile.replaceWithText(libBackup);
    else        libFile.deleteFile();

    return ok;
}
#endif

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
    ok = verifyManualUnpitchedPercussionPresets() && ok;
    ok = verifyCombiOverridesSectionPresets() && ok;
    ok = verifyUserCombiHarpPianoOverride() && ok;
    ok = verifyManualHarpPianoSliders() && ok;
    ok = verifyInstrumentGridUserCombi() && ok;
    ok = verifyRandomGridGenerator() && ok;
    ok = verifyUserCombiExplicitCcValues() && ok;
    ok = verifyGateResponseManualPanel() && ok;
#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    ok = verifyNarrativeScanDrivesCombiSend() && ok;
    ok = verifyNarrativeControlCcInput() && ok;
    ok = verifyInputPassthroughModes() && ok;
    ok = verifyGateResponseLanePoint() && ok;
#endif
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






