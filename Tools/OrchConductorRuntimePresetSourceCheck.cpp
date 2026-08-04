#include <JuceHeader.h>

#include "../Source/OrchConductorRuntimePresetSource.h"

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

bool verifyLoadedLibrary(const orchconductor::PresetLibraryDefinition& library)
{
    bool ok = true;

    ok = checkPass(library.schema == "orchconductor.library", "schema is orchconductor.library") && ok;
    ok = checkEquals(library.schemaVersion, 1, "schemaVersion") && ok;
    ok = checkPass(library.pluginTarget == "OrchConductor", "pluginTarget is OrchConductor") && ok;

    ok = checkEquals(library.midi.channel, 1, "midi.channel") && ok;
    ok = checkEquals(library.midi.ccMin, 20, "midi.ccMin") && ok;
    ok = checkEquals(library.midi.ccMax, 54, "midi.ccMax") && ok;

    ok = checkEquals(static_cast<int>(library.woodwindPresets.size()), 20, "woodwinds preset count") && ok;
    ok = checkEquals(static_cast<int>(library.brassPresets.size()), 17, "brass preset count") && ok;
    ok = checkEquals(static_cast<int>(library.percussionPresets.size()), 9, "percussion preset count") && ok;
    ok = checkEquals(static_cast<int>(library.stringPresets.size()), 14, "strings preset count") && ok;
    ok = checkEquals(static_cast<int>(library.combiPresets.size()), 29, "combi preset count") && ok;

    ok = checkPass(library.isValid(), "runtime preset source library isValid()") && ok;

    return ok;
}

} // namespace

int main()
{
    std::cout << "OrchConductor runtime preset source boundary check" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    const auto result = orchconductor::RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled();

    std::cout << "[INFO] Diagnostic: " << result.diagnosticMessage << std::endl;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    bool ok = true;

    ok = checkPass(result.wasLoaded(), "runtime JSON preset source loaded when feature gate is ON") && ok;
    ok = checkPass(! result.requiresHardcodedFallback(), "hardcoded fallback not required after successful ON load") && ok;

    if (result.wasLoaded())
        ok = verifyLoadedLibrary(result.library) && ok;

    if (! ok)
        return fail("Runtime preset source ON verification failed.");

    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "[PASS] Runtime preset source ON verification completed successfully." << std::endl;
#else
    bool ok = true;

    ok = checkPass(! result.wasLoaded(), "runtime JSON preset source does not load when feature gate is OFF") && ok;
    ok = checkPass(result.requiresHardcodedFallback(), "hardcoded fallback required when feature gate is OFF") && ok;
    ok = checkPass(result.diagnosticMessage.isNotEmpty(), "disabled diagnostic message is present") && ok;

    if (! ok)
        return fail("Runtime preset source OFF verification failed.");

    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "[PASS] Runtime preset source OFF verification completed successfully." << std::endl;
#endif

    return 0;
}
