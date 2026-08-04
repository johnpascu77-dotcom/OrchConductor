#include <JuceHeader.h>

#include "../Source/OrchConductorEmbeddedFactoryJson.h"
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

bool verifyCoreFactoryShape(const orchconductor::PresetLibraryDefinition& library)
{
    bool ok = true;

    ok = checkEquals(library.schema, "orchconductor.library", "schema") && ok;
    ok = checkEquals(library.schemaVersion, 1, "schemaVersion") && ok;
    ok = checkEquals(library.libraryName, "OrchConductor Factory Library", "libraryName") && ok;
    ok = checkEquals(library.pluginTarget, "OrchConductor", "pluginTarget") && ok;

    ok = checkEquals(library.midi.channel, 1, "midi.channel") && ok;
    ok = checkEquals(library.midi.ccMin, 20, "midi.ccMin") && ok;
    ok = checkEquals(library.midi.ccMax, 54, "midi.ccMax") && ok;

    ok = checkEquals(static_cast<int>(library.woodwindPresets.size()), 20, "woodwinds preset count") && ok;
    ok = checkEquals(static_cast<int>(library.brassPresets.size()), 17, "brass preset count") && ok;
    ok = checkEquals(static_cast<int>(library.percussionPresets.size()), 9, "percussion preset count") && ok;
    ok = checkEquals(static_cast<int>(library.stringPresets.size()), 14, "strings preset count") && ok;
    ok = checkEquals(static_cast<int>(library.combiPresets.size()), 29, "combi preset count") && ok;

    ok = checkEquals(static_cast<int>(library.midi.reservedControllers.size()), 1, "reserved controller count") && ok;

    if (! library.midi.reservedControllers.empty())
        ok = checkEquals(library.midi.reservedControllers.front().ccNumber, 49, "reserved controller CC") && ok;

    ok = check(library.isValid(), "Embedded library failed final isValid() check.") && ok;

    return ok;
}

} // namespace

int main()
{
    std::cout << "OrchConductor embedded factory JSON accessor check" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;

    const auto embeddedJson = orchconductor::getEmbeddedFactoryJson();

    bool ok = true;

    ok = checkPass(embeddedJson.data != nullptr, "embedded JSON pointer is non-null") && ok;
    ok = checkPass(embeddedJson.size > 0, "embedded JSON size is greater than zero") && ok;
    ok = checkPass(embeddedJson.isValid(), "embedded JSON accessor reports valid") && ok;

    if (! ok)
        return fail("Embedded JSON accessor returned invalid data.");

    std::cout << "[INFO] Embedded JSON size: " << embeddedJson.size << " bytes" << std::endl;

    const auto jsonText = juce::String::fromUTF8(embeddedJson.data, embeddedJson.size);

    if (jsonText.isEmpty())
        return fail("Embedded JSON converted to empty text.");

    std::cout << "[PASS] Embedded JSON converted to text" << std::endl;

    const auto loadResult = orchconductor::PresetLibraryJsonLoader::fromJsonText(jsonText);

    if (! loadResult.wasOk())
        return fail(loadResult.errorMessage);

    std::cout << "[PASS] Embedded JSON parsed by passive JSON loader" << std::endl;

    ok = verifyCoreFactoryShape(loadResult.library) && ok;

    if (! ok)
        return fail("One or more embedded JSON verification checks failed.");

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "[PASS] Embedded factory JSON accessor verification completed successfully." << std::endl;
    std::cout << "[PASS] Embedded factory JSON passive loader verification completed successfully." << std::endl;

    return 0;
}
