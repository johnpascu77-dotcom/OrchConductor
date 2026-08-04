#include <JuceHeader.h>

#include "../Source/OrchConductorRuntimePresetCatalog.h"
#include "../Source/OrchConductorRuntimePresetSource.h"

#include <iostream>

namespace
{

constexpr int expectedFactorySectionCount = 5;
constexpr int expectedFactoryWoodwindPresetCount = 20;
constexpr int expectedFactoryBrassPresetCount = 17;
constexpr int expectedFactoryPercussionPresetCount = 9;
constexpr int expectedFactoryStringPresetCount = 14;
constexpr int expectedFactoryCombiPresetCount = 29;

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

bool verifyExpectedFactoryShape(const OrchConductorRuntimePresetCatalog& catalog,
                                const juce::String& labelPrefix)
{
    bool ok = true;

    ok = checkPass(catalog.hasExpectedFactoryShape(), labelPrefix + " has expected factory shape") && ok;
    ok = checkPass(catalog.getSectionCount() == expectedFactorySectionCount,
                   labelPrefix + " section count matches factory shape") && ok;
    ok = checkPass(catalog.getSectionPresetCount("woodwinds") == expectedFactoryWoodwindPresetCount,
                   labelPrefix + " woodwind preset count matches factory shape") && ok;
    ok = checkPass(catalog.getSectionPresetCount("brass") == expectedFactoryBrassPresetCount,
                   labelPrefix + " brass preset count matches factory shape") && ok;
    ok = checkPass(catalog.getSectionPresetCount("percussion") == expectedFactoryPercussionPresetCount,
                   labelPrefix + " percussion preset count matches factory shape") && ok;
    ok = checkPass(catalog.getSectionPresetCount("strings") == expectedFactoryStringPresetCount,
                   labelPrefix + " string preset count matches factory shape") && ok;
    ok = checkPass(catalog.getCombiPresetCount() == expectedFactoryCombiPresetCount,
                   labelPrefix + " combi preset count matches factory shape") && ok;

    return ok;
}

bool verifySafeLabelAccess(const OrchConductorRuntimePresetCatalog& catalog,
                           const juce::String& labelPrefix)
{
    bool ok = true;

    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", 0).isNotEmpty(),
                   labelPrefix + " first woodwind label is non-empty") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("brass", 0).isNotEmpty(),
                   labelPrefix + " first brass label is non-empty") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("percussion", 0).isNotEmpty(),
                   labelPrefix + " first percussion label is non-empty") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("strings", 0).isNotEmpty(),
                   labelPrefix + " first string label is non-empty") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(0).isNotEmpty(),
                   labelPrefix + " first combi label is non-empty") && ok;

    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", -1).isEmpty(),
                   labelPrefix + " negative section label index returns empty") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", 9999).isEmpty(),
                   labelPrefix + " out-of-range section label index returns empty") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("unknown", 0).isEmpty(),
                   labelPrefix + " unknown section label returns empty") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(-1).isEmpty(),
                   labelPrefix + " negative combi label index returns empty") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(9999).isEmpty(),
                   labelPrefix + " out-of-range combi label index returns empty") && ok;

    return ok;
}

bool verifyRuntimeSourceLabelsMirrorSource(const orchconductor::RuntimePresetSourceResult& source,
                                           const OrchConductorRuntimePresetCatalog& catalog)
{
    bool ok = true;

    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", 0) == source.library.woodwindPresets.front().name,
                   "runtime-source catalog mirrors first woodwind label") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("brass", 0) == source.library.brassPresets.front().name,
                   "runtime-source catalog mirrors first brass label") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("percussion", 0) == source.library.percussionPresets.front().name,
                   "runtime-source catalog mirrors first percussion label") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("strings", 0) == source.library.stringPresets.front().name,
                   "runtime-source catalog mirrors first string label") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(0) == source.library.combiPresets.front().name,
                   "runtime-source catalog mirrors first combi label") && ok;

    return ok;
}

bool verifyFallbackCatalog()
{
    const auto catalog = OrchConductorRuntimePresetCatalog::createFallbackCatalog();

    bool ok = true;

    ok = checkPass(catalog.isReady(), "fallback catalog is ready") && ok;
    ok = checkPass(catalog.requiresFallback(), "fallback catalog requires fallback") && ok;
    ok = checkPass(catalog.getDiagnosticMessage().isNotEmpty(), "fallback catalog diagnostic is non-empty") && ok;
    ok = verifyExpectedFactoryShape(catalog, "fallback catalog") && ok;
    ok = verifySafeLabelAccess(catalog, "fallback catalog") && ok;

    return ok;
}

bool verifyRuntimeSourceCatalog()
{
    const auto source = orchconductor::RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled();
    const auto catalog = OrchConductorRuntimePresetCatalog::createFromRuntimeSource(source);

    bool ok = true;

    ok = checkPass(catalog.isReady(), "runtime-source catalog is ready") && ok;
    ok = checkPass(catalog.getDiagnosticMessage().isNotEmpty(), "runtime-source catalog diagnostic is non-empty") && ok;
    ok = verifyExpectedFactoryShape(catalog, "runtime-source catalog") && ok;
    ok = verifySafeLabelAccess(catalog, "runtime-source catalog") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    ok = checkPass(source.wasLoaded(), "runtime source loaded in ON catalog check build") && ok;
    ok = checkPass(! source.requiresHardcodedFallback(), "runtime source does not require fallback in ON catalog check build") && ok;
    ok = checkPass(! catalog.requiresFallback(), "runtime-source catalog does not require fallback in ON catalog check build") && ok;
    ok = verifyRuntimeSourceLabelsMirrorSource(source, catalog) && ok;
#else
    ok = checkPass(! source.wasLoaded(), "runtime source not loaded in OFF catalog check build") && ok;
    ok = checkPass(source.requiresHardcodedFallback(), "runtime source requires fallback in OFF catalog check build") && ok;
    ok = checkPass(catalog.requiresFallback(), "runtime-source catalog requires fallback in OFF catalog check build") && ok;
#endif

    return ok;
}

} // namespace

int main()
{
    std::cout << "OrchConductor runtime preset catalog label access trial check" << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    std::cout << "[INFO] Runtime JSON preset feature gate: ON" << std::endl;
#else
    std::cout << "[INFO] Runtime JSON preset feature gate: OFF" << std::endl;
#endif

    bool ok = true;

    ok = verifyFallbackCatalog() && ok;
    ok = verifyRuntimeSourceCatalog() && ok;

    if (! ok)
        return fail("Runtime preset catalog label access trial verification failed.");

    std::cout << "------------------------------------------------------------" << std::endl;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    std::cout << "[PASS] Runtime preset catalog label access trial completed successfully with runtime JSON ON." << std::endl;
#else
    std::cout << "[PASS] Runtime preset catalog label access trial completed successfully with runtime JSON OFF." << std::endl;
#endif

    std::cout << "[PASS] Catalog label access remains non-authoritative." << std::endl;

    return 0;
}
