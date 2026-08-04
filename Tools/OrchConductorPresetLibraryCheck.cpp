#include <JuceHeader.h>

#include "../Source/OrchConductorPresetLibraryJson.h"

#include <iostream>

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

    ok = checkEquals(static_cast<int>(library.midi.reservedControllers.size()), 1, "reserved controller count") && ok;

    if (! library.midi.reservedControllers.empty())
    {
        const auto& reserved = library.midi.reservedControllers.front();

        ok = checkEquals(reserved.ccNumber, 49, "reserved controller CC") && ok;
        ok = checkEquals(reserved.name, "Harp", "reserved controller name") && ok;
        ok = checkEquals(reserved.status, "reserved", "reserved controller status") && ok;
        ok = checkEquals(reserved.defaultValue, 0, "reserved controller default value") && ok;
    }

    ok = checkEquals(static_cast<int>(library.woodwindPresets.size()), 20, "woodwind preset count") && ok;
    ok = checkEquals(static_cast<int>(library.brassPresets.size()), 17, "brass preset count") && ok;
    ok = checkEquals(static_cast<int>(library.percussionPresets.size()), 9, "percussion preset count") && ok;
    ok = checkEquals(static_cast<int>(library.stringPresets.size()), 14, "string preset count") && ok;
    ok = checkEquals(static_cast<int>(library.combiPresets.size()), 29, "combi preset count") && ok;

    ok = check(library.isValid(), "Loaded library failed final isValid() check.") && ok;

    if (! ok)
        return fail("One or more loader verification checks failed.");

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "[PASS] Passive JSON loader verification completed successfully." << std::endl;

    return 0;
}
