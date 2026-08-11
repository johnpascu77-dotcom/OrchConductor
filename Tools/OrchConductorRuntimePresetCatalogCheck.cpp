#include <JuceHeader.h>

#include "../Source/OrchConductorRuntimePresetCatalog.h"
#include "../Source/OrchConductorRuntimePresetSource.h"

#include <iostream>

namespace
{

constexpr int expectedFactorySectionCount = 4;
constexpr int expectedFactoryWoodwindPresetCount = 20;
constexpr int expectedFactoryBrassPresetCount = 17;
constexpr int expectedFactoryPercussionPresetCount = 9;
constexpr int expectedFactoryStringPresetCount = 14;
constexpr int expectedFactoryCombiPresetCount = 29;

const juce::String expectedWoodwindFirstLabel = "All Off";
const juce::String expectedWoodwindLastLabel = "Full Woodwinds";
const juce::String expectedBrassFirstLabel = "All Off";
const juce::String expectedBrassLastLabel = "Full Brass";
const juce::String expectedPercussionFirstLabel = "All Off";
const juce::String expectedPercussionLastLabel = "Full Melodic Percussion";
const juce::String expectedStringFirstLabel = "All Off";
const juce::String expectedStringLastLabel = "Tutti";
const juce::String expectedCombiFirstLabel = "Manual Sections";
const juce::String expectedCombiLastLabel = "[Solo] English Horn Lament";

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
    ok = checkPass(catalog.getSectionPresetCount("unknown") == 0,
                   labelPrefix + " unknown section count returns zero") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(-1).isEmpty(),
                   labelPrefix + " negative combi label index returns empty") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(9999).isEmpty(),
                   labelPrefix + " out-of-range combi label index returns empty") && ok;

    return ok;
}


bool verifySafeValueAccess(const OrchConductorRuntimePresetCatalog& catalog,
                           const juce::String& labelPrefix)
{
    bool ok = true;

    ok = checkPass(catalog.getSectionPresetValueCount("woodwinds", -1) == 0,
                   labelPrefix + " negative section preset value count returns zero") && ok;
    ok = checkPass(catalog.getSectionPresetValueCount("woodwinds", 9999) == 0,
                   labelPrefix + " out-of-range section preset value count returns zero") && ok;
    ok = checkPass(catalog.getSectionPresetValueCount("unknown", 0) == 0,
                   labelPrefix + " unknown section preset value count returns zero") && ok;

    ok = checkPass(catalog.getCombiPresetValueCount(-1) == 0,
                   labelPrefix + " negative combi preset value count returns zero") && ok;
    ok = checkPass(catalog.getCombiPresetValueCount(9999) == 0,
                   labelPrefix + " out-of-range combi preset value count returns zero") && ok;

    ok = checkPass(! catalog.getSectionPresetValue("woodwinds", -1, 0).isValid,
                   labelPrefix + " negative section preset value returns invalid view") && ok;
    ok = checkPass(! catalog.getSectionPresetValue("woodwinds", 0, -1).isValid,
                   labelPrefix + " negative section value index returns invalid view") && ok;
    ok = checkPass(! catalog.getSectionPresetValue("woodwinds", 9999, 0).isValid,
                   labelPrefix + " out-of-range section preset value returns invalid view") && ok;
    ok = checkPass(! catalog.getSectionPresetValue("woodwinds", 0, 9999).isValid,
                   labelPrefix + " out-of-range section value index returns invalid view") && ok;
    ok = checkPass(! catalog.getSectionPresetValue("unknown", 0, 0).isValid,
                   labelPrefix + " unknown section value returns invalid view") && ok;

    ok = checkPass(! catalog.getCombiPresetValue(-1, 0).isValid,
                   labelPrefix + " negative combi preset value returns invalid view") && ok;
    ok = checkPass(! catalog.getCombiPresetValue(0, -1).isValid,
                   labelPrefix + " negative combi value index returns invalid view") && ok;
    ok = checkPass(! catalog.getCombiPresetValue(9999, 0).isValid,
                   labelPrefix + " out-of-range combi preset value returns invalid view") && ok;
    ok = checkPass(! catalog.getCombiPresetValue(0, 9999).isValid,
                   labelPrefix + " out-of-range combi value index returns invalid view") && ok;

    return ok;
}

bool verifyRuntimeSourceValuesMirrorSource(const orchconductor::RuntimePresetSourceResult& source,
                                           const OrchConductorRuntimePresetCatalog& catalog)
{
    bool ok = true;

    ok = checkPass(catalog.getSectionPresetValueCount("woodwinds", 0) == static_cast<int>(source.library.woodwindPresets.front().values.size()),
                   "runtime-source catalog first woodwind value count mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetValueCount("brass", 0) == static_cast<int>(source.library.brassPresets.front().values.size()),
                   "runtime-source catalog first brass value count mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetValueCount("percussion", 0) == static_cast<int>(source.library.percussionPresets.front().values.size()),
                   "runtime-source catalog first percussion value count mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetValueCount("strings", 0) == static_cast<int>(source.library.stringPresets.front().values.size()),
                   "runtime-source catalog first string value count mirrors source") && ok;
    ok = checkPass(catalog.getCombiPresetValueCount(0) == static_cast<int>(source.library.combiPresets.front().values.size()),
                   "runtime-source catalog first combi value count mirrors source") && ok;

    if (! source.library.woodwindPresets.front().values.empty())
    {
        const auto catalogValue = catalog.getSectionPresetValue("woodwinds", 0, 0);
        const auto& sourceValue = source.library.woodwindPresets.front().values.front();

        ok = checkPass(catalogValue.isValid,
                       "runtime-source catalog first woodwind value is valid") && ok;
        ok = checkPass(catalogValue.ccNumber == sourceValue.ccNumber,
                       "runtime-source catalog first woodwind value CC mirrors source") && ok;
        ok = checkPass(catalogValue.value == sourceValue.value,
                       "runtime-source catalog first woodwind value amount mirrors source") && ok;
    }

    if (! source.library.combiPresets.front().values.empty())
    {
        const auto catalogValue = catalog.getCombiPresetValue(0, 0);
        const auto& sourceValue = source.library.combiPresets.front().values.front();

        ok = checkPass(catalogValue.isValid,
                       "runtime-source catalog first combi value is valid") && ok;
        ok = checkPass(catalogValue.ccNumber == sourceValue.ccNumber,
                       "runtime-source catalog first combi value CC mirrors source") && ok;
        ok = checkPass(catalogValue.value == sourceValue.value,
                       "runtime-source catalog first combi value amount mirrors source") && ok;
    }

    return ok;
}
bool verifyRuntimeSourceLabelsMirrorSource(const orchconductor::RuntimePresetSourceResult& source,
                                           const OrchConductorRuntimePresetCatalog& catalog)
{
    bool ok = true;

    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", 0) == source.library.woodwindPresets.front().name,
                   "runtime-source catalog mirrors first woodwind label from source") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("brass", 0) == source.library.brassPresets.front().name,
                   "runtime-source catalog mirrors first brass label from source") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("percussion", 0) == source.library.percussionPresets.front().name,
                   "runtime-source catalog mirrors first percussion label from source") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("strings", 0) == source.library.stringPresets.front().name,
                   "runtime-source catalog mirrors first string label from source") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(0) == source.library.combiPresets.front().name,
                   "runtime-source catalog mirrors first combi label from source") && ok;

    return ok;
}

bool verifyRuntimeSourceLabelsMatchExpectedFactorySnapshot(const OrchConductorRuntimePresetCatalog& catalog)
{
    bool ok = true;

    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", 0) == expectedWoodwindFirstLabel,
                   "runtime-source catalog first woodwind label matches expected factory snapshot") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", expectedFactoryWoodwindPresetCount - 1) == expectedWoodwindLastLabel,
                   "runtime-source catalog last woodwind label matches expected factory snapshot") && ok;

    ok = checkPass(catalog.getSectionPresetLabel("brass", 0) == expectedBrassFirstLabel,
                   "runtime-source catalog first brass label matches expected factory snapshot") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("brass", expectedFactoryBrassPresetCount - 1) == expectedBrassLastLabel,
                   "runtime-source catalog last brass label matches expected factory snapshot") && ok;

    ok = checkPass(catalog.getSectionPresetLabel("percussion", 0) == expectedPercussionFirstLabel,
                   "runtime-source catalog first percussion label matches expected factory snapshot") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("percussion", expectedFactoryPercussionPresetCount - 1) == expectedPercussionLastLabel,
                   "runtime-source catalog last percussion label matches expected factory snapshot") && ok;

    ok = checkPass(catalog.getSectionPresetLabel("strings", 0) == expectedStringFirstLabel,
                   "runtime-source catalog first string label matches expected factory snapshot") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("strings", expectedFactoryStringPresetCount - 1) == expectedStringLastLabel,
                   "runtime-source catalog last string label matches expected factory snapshot") && ok;

    ok = checkPass(catalog.getCombiPresetLabel(0) == expectedCombiFirstLabel,
                   "runtime-source catalog first combi label matches expected factory snapshot") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(expectedFactoryCombiPresetCount - 1) == expectedCombiLastLabel,
                   "runtime-source catalog last combi label matches expected factory snapshot") && ok;

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
    ok = verifySafeValueAccess(catalog, "fallback catalog") && ok;

    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", 0) != expectedWoodwindFirstLabel,
                   "fallback catalog does not claim real factory woodwind labels") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(0) != expectedCombiFirstLabel,
                   "fallback catalog does not claim real factory combi labels") && ok;

    return ok;
}

bool verifyControlledRuntimeJsonAuthorityTrial(const orchconductor::RuntimePresetSourceResult& source,
                                               const OrchConductorRuntimePresetCatalog& catalog)
{
    bool ok = true;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    ok = checkPass(source.wasLoaded(),
                   "controlled runtime JSON authority trial source is loaded with runtime JSON ON") && ok;
    ok = checkPass(! source.requiresHardcodedFallback(),
                   "controlled runtime JSON authority trial source does not require fallback with runtime JSON ON") && ok;
    ok = checkPass(catalog.isReady(),
                   "controlled runtime JSON authority trial catalog is ready with runtime JSON ON") && ok;
    ok = checkPass(! catalog.requiresFallback(),
                   "controlled runtime JSON authority trial catalog does not require fallback with runtime JSON ON") && ok;
    ok = checkPass(catalog.hasExpectedFactoryShape(),
                   "controlled runtime JSON authority trial catalog has expected factory shape with runtime JSON ON") && ok;

    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", 0) == source.library.woodwindPresets.front().name,
                   "controlled runtime JSON authority trial woodwind label payload mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("brass", 0) == source.library.brassPresets.front().name,
                   "controlled runtime JSON authority trial brass label payload mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("percussion", 0) == source.library.percussionPresets.front().name,
                   "controlled runtime JSON authority trial percussion label payload mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("strings", 0) == source.library.stringPresets.front().name,
                   "controlled runtime JSON authority trial string label payload mirrors source") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(0) == source.library.combiPresets.front().name,
                   "controlled runtime JSON authority trial combi label payload mirrors source") && ok;

    ok = checkPass(catalog.getSectionPresetValueCount("woodwinds", 0) == static_cast<int>(source.library.woodwindPresets.front().values.size()),
                   "controlled runtime JSON authority trial woodwind value payload count mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetValueCount("brass", 0) == static_cast<int>(source.library.brassPresets.front().values.size()),
                   "controlled runtime JSON authority trial brass value payload count mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetValueCount("percussion", 0) == static_cast<int>(source.library.percussionPresets.front().values.size()),
                   "controlled runtime JSON authority trial percussion value payload count mirrors source") && ok;
    ok = checkPass(catalog.getSectionPresetValueCount("strings", 0) == static_cast<int>(source.library.stringPresets.front().values.size()),
                   "controlled runtime JSON authority trial string value payload count mirrors source") && ok;
    ok = checkPass(catalog.getCombiPresetValueCount(0) == static_cast<int>(source.library.combiPresets.front().values.size()),
                   "controlled runtime JSON authority trial combi value payload count mirrors source") && ok;

    if (! source.library.woodwindPresets.front().values.empty())
    {
        const auto catalogValue = catalog.getSectionPresetValue("woodwinds", 0, 0);
        const auto& sourceValue = source.library.woodwindPresets.front().values.front();

        ok = checkPass(catalogValue.isValid,
                       "controlled runtime JSON authority trial woodwind typed value payload is valid") && ok;
        ok = checkPass(catalogValue.ccNumber == sourceValue.ccNumber,
                       "controlled runtime JSON authority trial woodwind typed value CC mirrors source") && ok;
        ok = checkPass(catalogValue.value == sourceValue.value,
                       "controlled runtime JSON authority trial woodwind typed value amount mirrors source") && ok;
    }

    if (! source.library.combiPresets.front().values.empty())
    {
        const auto catalogValue = catalog.getCombiPresetValue(0, 0);
        const auto& sourceValue = source.library.combiPresets.front().values.front();

        ok = checkPass(catalogValue.isValid,
                       "controlled runtime JSON authority trial combi typed value payload is valid") && ok;
        ok = checkPass(catalogValue.ccNumber == sourceValue.ccNumber,
                       "controlled runtime JSON authority trial combi typed value CC mirrors source") && ok;
        ok = checkPass(catalogValue.value == sourceValue.value,
                       "controlled runtime JSON authority trial combi typed value amount mirrors source") && ok;
    }

    ok = checkPass(ok,
                   "controlled runtime JSON authority trial payload parity verified with runtime JSON ON") && ok;
#else
    ok = checkPass(! source.wasLoaded(),
                   "controlled runtime JSON authority trial source is not loaded with runtime JSON OFF") && ok;
    ok = checkPass(source.requiresHardcodedFallback(),
                   "controlled runtime JSON authority trial source requires fallback with runtime JSON OFF") && ok;
    ok = checkPass(catalog.requiresFallback(),
                   "controlled runtime JSON authority trial catalog remains fallback-only with runtime JSON OFF") && ok;
    ok = checkPass(ok,
                   "controlled runtime JSON authority trial inactive with runtime JSON OFF") && ok;
#endif

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
    ok = verifySafeValueAccess(catalog, "runtime-source catalog") && ok;
    ok = verifyControlledRuntimeJsonAuthorityTrial(source, catalog) && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    ok = checkPass(source.wasLoaded(), "runtime source loaded in ON catalog check build") && ok;
    ok = checkPass(! source.requiresHardcodedFallback(), "runtime source does not require fallback in ON catalog check build") && ok;
    ok = checkPass(! catalog.requiresFallback(), "runtime-source catalog does not require fallback in ON catalog check build") && ok;
    ok = verifyRuntimeSourceLabelsMirrorSource(source, catalog) && ok;
    ok = verifyRuntimeSourceLabelsMatchExpectedFactorySnapshot(catalog) && ok;
    ok = verifyRuntimeSourceValuesMirrorSource(source, catalog) && ok;
#else
    ok = checkPass(! source.wasLoaded(), "runtime source not loaded in OFF catalog check build") && ok;
    ok = checkPass(source.requiresHardcodedFallback(), "runtime source requires fallback in OFF catalog check build") && ok;
    ok = checkPass(catalog.requiresFallback(), "runtime-source catalog requires fallback in OFF catalog check build") && ok;
    ok = checkPass(catalog.getSectionPresetLabel("woodwinds", 0) != expectedWoodwindFirstLabel,
                   "OFF runtime-source catalog does not claim real factory woodwind labels") && ok;
    ok = checkPass(catalog.getCombiPresetLabel(0) != expectedCombiFirstLabel,
                   "OFF runtime-source catalog does not claim real factory combi labels") && ok;
#endif

    return ok;
}

} // namespace

int main()
{
    std::cout << "OrchConductor controlled runtime JSON authority trial check" << std::endl;
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
        return fail("Runtime preset catalog value access and parity verification failed.");

    std::cout << "------------------------------------------------------------" << std::endl;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    std::cout << "[PASS] Runtime preset catalog value access and parity verification completed successfully with runtime JSON ON." << std::endl;
#else
    std::cout << "[PASS] Runtime preset catalog value access and parity verification completed successfully with runtime JSON OFF." << std::endl;
#endif

    std::cout << "[PASS] Catalog value access and parity remains non-authoritative." << std::endl;
    std::cout << "[PASS] Phase 4P final non-authoritative runtime catalog parity summary satisfied." << std::endl;
    std::cout << "[PASS] Phase 5A controlled runtime JSON authority trial satisfied." << std::endl;

    return 0;
}

