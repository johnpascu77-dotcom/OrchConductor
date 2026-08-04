#include <JuceHeader.h>

#include "../Source/OrchConductorPresetLibraryJson.h"

#include <algorithm>
#include <iostream>
#include <set>

namespace
{

int fail(const juce::String& message)
{
    std::cerr << "[FAIL] " << message << std::endl;
    return 1;
}

bool check(bool condition, const juce::String& message)
{
    if (! condition)
        std::cerr << "[FAIL] " << message << std::endl;

    return condition;
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

bool checkEquals(const juce::String& actual, const juce::String& expected, const juce::String& label)
{
    if (actual != expected)
    {
        std::cerr << "[FAIL] " << label
                  << ": expected '" << expected
                  << "', got '" << actual
                  << "'"
                  << std::endl;
        return false;
    }

    std::cout << "[PASS] " << label << ": " << actual << std::endl;
    return true;
}

juce::File findRepositoryRoot()
{
    auto current = juce::File::getCurrentWorkingDirectory();

    for (int i = 0; i < 12; ++i)
    {
        if (current.getChildFile("Examples")
                   .getChildFile("orchconductor_library_v1.example.json")
                   .existsAsFile())
        {
            return current;
        }

        const auto parent = current.getParentDirectory();

        if (parent == current)
            break;

        current = parent;
    }

    return {};
}

const orchconductor::SectionPresetDefinition* findSectionPreset(
    const std::vector<orchconductor::SectionPresetDefinition>& presets,
    const juce::String& id)
{
    const auto it = std::find_if(
        presets.begin(),
        presets.end(),
        [&id](const auto& preset)
        {
            return preset.id == id;
        });

    return it == presets.end() ? nullptr : &(*it);
}

const orchconductor::CombiPresetDefinition* findCombiPreset(
    const std::vector<orchconductor::CombiPresetDefinition>& presets,
    const juce::String& id)
{
    const auto it = std::find_if(
        presets.begin(),
        presets.end(),
        [&id](const auto& preset)
        {
            return preset.id == id;
        });

    return it == presets.end() ? nullptr : &(*it);
}

bool hasValue(const std::vector<orchconductor::PresetValue>& values, int ccNumber, int value)
{
    return std::any_of(
        values.begin(),
        values.end(),
        [ccNumber, value](const auto& presetValue)
        {
            return presetValue.ccNumber == ccNumber && presetValue.value == value;
        });
}

bool verifyPresetValues(const std::vector<orchconductor::PresetValue>& values,
                        const juce::String& context,
                        int ccMin,
                        int ccMax,
                        int reservedCc)
{
    bool ok = true;

    for (const auto& value : values)
    {
        ok = check(value.ccNumber >= ccMin && value.ccNumber <= ccMax,
                   context + " contains CC outside configured range: " + juce::String(value.ccNumber)) && ok;

        ok = check(value.ccNumber != reservedCc,
                   context + " targets reserved CC" + juce::String(reservedCc)) && ok;

        ok = check(value.value >= 0 && value.value <= 127,
                   context + " contains value outside 0..127 for CC" + juce::String(value.ccNumber)) && ok;
    }

    return ok;
}

bool verifySectionPresetDomain(const std::vector<orchconductor::SectionPresetDefinition>& presets,
                               const juce::String& domainName,
                               int expectedCount,
                               int ccMin,
                               int ccMax,
                               int reservedCc)
{
    bool ok = true;

    ok = checkEquals(static_cast<int>(presets.size()), expectedCount, domainName + " preset count") && ok;

    std::set<juce::String> ids;

    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
    {
        const auto& preset = presets[static_cast<size_t>(i)];
        const auto context = domainName + "[" + juce::String(i) + "]";

        ok = check(! preset.id.isEmpty(), context + " has an empty id.") && ok;
        ok = check(ids.insert(preset.id).second, context + " has duplicate id: " + preset.id) && ok;
        ok = checkEquals(preset.factoryId, i, context + " factoryId") && ok;
        ok = checkEquals(preset.section, domainName, context + " section") && ok;
        ok = verifyPresetValues(preset.values, context, ccMin, ccMax, reservedCc) && ok;
    }

    if (ok)
        std::cout << "[PASS] " << domainName << " parity checks" << std::endl;

    return ok;
}

bool verifyCombiPresetDomain(const std::vector<orchconductor::CombiPresetDefinition>& presets,
                             int expectedCount,
                             int ccMin,
                             int ccMax,
                             int reservedCc)
{
    bool ok = true;

    ok = checkEquals(static_cast<int>(presets.size()), expectedCount, "combi preset count") && ok;

    std::set<juce::String> ids;

    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
    {
        const auto& preset = presets[static_cast<size_t>(i)];
        const auto context = "combiPresets[" + juce::String(i) + "]";

        ok = check(! preset.id.isEmpty(), context + " has an empty id.") && ok;
        ok = check(ids.insert(preset.id).second, context + " has duplicate id: " + preset.id) && ok;
        ok = checkEquals(preset.factoryId, i, context + " factoryId") && ok;
        ok = verifyPresetValues(preset.values, context, ccMin, ccMax, reservedCc) && ok;
    }

    if (ok)
        std::cout << "[PASS] combi parity checks" << std::endl;

    return ok;
}

bool verifyKnownFactoryEntries(const orchconductor::PresetLibraryDefinition& library)
{
    bool ok = true;

    const auto* woodwindsAllOff = findSectionPreset(library.woodwindPresets, "woodwinds.all_off");
    const auto* brassAllOff = findSectionPreset(library.brassPresets, "brass.all_off");
    const auto* percussionAllOff = findSectionPreset(library.percussionPresets, "percussion.all_off");
    const auto* stringsAllOff = findSectionPreset(library.stringPresets, "strings.all_off");
    const auto* manualSections = findCombiPreset(library.combiPresets, "manual.sections");

    ok = checkPass(woodwindsAllOff != nullptr, "known preset exists: woodwinds.all_off") && ok;
    ok = checkPass(brassAllOff != nullptr, "known preset exists: brass.all_off") && ok;
    ok = checkPass(percussionAllOff != nullptr, "known preset exists: percussion.all_off") && ok;
    ok = checkPass(stringsAllOff != nullptr, "known preset exists: strings.all_off") && ok;
    ok = checkPass(manualSections != nullptr, "known combi exists: manual.sections") && ok;

    if (woodwindsAllOff != nullptr)
        ok = checkEquals(static_cast<int>(woodwindsAllOff->values.size()), 0, "woodwinds.all_off value count") && ok;

    if (brassAllOff != nullptr)
        ok = checkEquals(static_cast<int>(brassAllOff->values.size()), 0, "brass.all_off value count") && ok;

    if (percussionAllOff != nullptr)
        ok = checkEquals(static_cast<int>(percussionAllOff->values.size()), 0, "percussion.all_off value count") && ok;

    if (stringsAllOff != nullptr)
        ok = checkEquals(static_cast<int>(stringsAllOff->values.size()), 0, "strings.all_off value count") && ok;

    if (manualSections != nullptr)
        ok = checkEquals(static_cast<int>(manualSections->values.size()), 0, "manual.sections value count") && ok;

    return ok;
}

bool verifySpecialPartialValueCases(const orchconductor::PresetLibraryDefinition& library)
{
    bool ok = true;

    const auto* lowStrings = findSectionPreset(library.stringPresets, "strings.low_strings");
    ok = checkPass(lowStrings != nullptr, "special preset exists: strings.low_strings") && ok;

    if (lowStrings != nullptr)
        ok = checkPass(hasValue(lowStrings->values, 52, 64), "strings.low_strings contains CC52 = 64") && ok;

    const auto* englishHornLament = findCombiPreset(library.combiPresets, "solo.english_horn_lament");
    ok = checkPass(englishHornLament != nullptr, "special combi exists: solo.english_horn_lament") && ok;

    if (englishHornLament != nullptr)
        ok = checkPass(hasValue(englishHornLament->values, 54, 64), "solo.english_horn_lament contains CC54 = 64") && ok;

    return ok;
}

bool verifyReservedControllerPolicy(const orchconductor::PresetLibraryDefinition& library)
{
    bool ok = true;

    ok = checkEquals(static_cast<int>(library.midi.reservedControllers.size()), 1, "reserved controller count") && ok;

    if (! library.midi.reservedControllers.empty())
    {
        const auto& reserved = library.midi.reservedControllers.front();

        ok = checkEquals(reserved.ccNumber, 49, "reserved controller CC") && ok;
        ok = checkEquals(reserved.name, "Harp", "reserved controller name") && ok;
        ok = checkEquals(reserved.status, "reserved", "reserved controller status") && ok;
        ok = checkEquals(reserved.defaultValue, 0, "reserved controller default value") && ok;
    }

    return ok;
}

} // namespace

int main()
{
    std::cout << "OrchConductor preset library loader check" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    const auto repositoryRoot = findRepositoryRoot();

    if (! repositoryRoot.exists())
        return fail("Could not locate repository root from current working directory.");

    const auto jsonFile = repositoryRoot
        .getChildFile("Examples")
        .getChildFile("orchconductor_library_v1.example.json");

    if (! jsonFile.existsAsFile())
        return fail("Factory JSON example file was not found: " + jsonFile.getFullPathName());

    std::cout << "[INFO] Loading: " << jsonFile.getFullPathName() << std::endl;

    const auto loadResult = orchconductor::PresetLibraryJsonLoader::fromJsonFile(jsonFile);

    if (! loadResult.wasOk())
        return fail(loadResult.errorMessage);

    const auto& library = loadResult.library;

    bool ok = true;

    ok = checkEquals(library.schema, "orchconductor.library", "schema") && ok;
    ok = checkEquals(library.schemaVersion, 1, "schemaVersion") && ok;
    ok = checkEquals(library.libraryName, "OrchConductor Factory Library", "libraryName") && ok;
    ok = checkEquals(library.pluginTarget, "OrchConductor", "pluginTarget") && ok;

    ok = checkEquals(library.midi.channel, 1, "midi.channel") && ok;
    ok = checkEquals(library.midi.ccMin, 20, "midi.ccMin") && ok;
    ok = checkEquals(library.midi.ccMax, 54, "midi.ccMax") && ok;

    constexpr int reservedCc = 49;

    ok = verifyReservedControllerPolicy(library) && ok;

    ok = verifySectionPresetDomain(library.woodwindPresets,
                                   "woodwinds",
                                   20,
                                   library.midi.ccMin,
                                   library.midi.ccMax,
                                   reservedCc) && ok;

    ok = verifySectionPresetDomain(library.brassPresets,
                                   "brass",
                                   17,
                                   library.midi.ccMin,
                                   library.midi.ccMax,
                                   reservedCc) && ok;

    ok = verifySectionPresetDomain(library.percussionPresets,
                                   "percussion",
                                   9,
                                   library.midi.ccMin,
                                   library.midi.ccMax,
                                   reservedCc) && ok;

    ok = verifySectionPresetDomain(library.stringPresets,
                                   "strings",
                                   14,
                                   library.midi.ccMin,
                                   library.midi.ccMax,
                                   reservedCc) && ok;

    ok = verifyCombiPresetDomain(library.combiPresets,
                                 29,
                                 library.midi.ccMin,
                                 library.midi.ccMax,
                                 reservedCc) && ok;

    ok = verifyKnownFactoryEntries(library) && ok;
    ok = verifySpecialPartialValueCases(library) && ok;

    ok = check(library.isValid(), "Loaded library failed final isValid() check.") && ok;

    if (! ok)
        return fail("One or more loader/parity verification checks failed.");

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[PASS] Passive JSON loader verification completed successfully." << std::endl;
    std::cout << "[PASS] Factory JSON parity verification completed successfully." << std::endl;

    return 0;
}
