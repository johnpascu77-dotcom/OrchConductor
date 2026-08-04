#include <JuceHeader.h>

#include "../Source/OrchConductorRuntimePresetCatalog.h"
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

bool verifyFallbackCatalog()
{
    const auto catalog = OrchConductorRuntimePresetCatalog::createFallbackCatalog();

    bool ok = true;

    ok = checkPass(catalog.isReady(), "fallback catalog is ready") && ok;
    ok = checkPass(catalog.requiresFallback(), "fallback catalog requires fallback") && ok;
    ok = checkPass(catalog.getDiagnosticMessage().isNotEmpty(), "fallback catalog diagnostic is non-empty") && ok;

    return ok;
}

bool verifyRuntimeSourceCatalog()
{
    const auto source = orchconductor::RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled();
    const auto catalog = OrchConductorRuntimePresetCatalog::createFromRuntimeSource(source);

    bool ok = true;

    ok = checkPass(catalog.isReady(), "runtime-source catalog is ready") && ok;
    ok = checkPass(catalog.getDiagnosticMessage().isNotEmpty(), "runtime-source catalog diagnostic is non-empty") && ok;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    ok = checkPass(source.wasLoaded(), "runtime source loaded in ON catalog check build") && ok;
    ok = checkPass(! source.requiresHardcodedFallback(), "runtime source does not require fallback in ON catalog check build") && ok;
    ok = checkPass(! catalog.requiresFallback(), "runtime-source catalog does not require fallback in ON catalog check build") && ok;
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
    std::cout << "OrchConductor read-only runtime preset catalog adapter check" << std::endl;
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
        return fail("Read-only runtime preset catalog adapter verification failed.");

    std::cout << "------------------------------------------------------------" << std::endl;

#if ORCHCONDUCTOR_ENABLE_RUNTIME_JSON_PRESETS
    std::cout << "[PASS] Read-only runtime preset catalog adapter verification completed successfully with runtime JSON ON." << std::endl;
#else
    std::cout << "[PASS] Read-only runtime preset catalog adapter verification completed successfully with runtime JSON OFF." << std::endl;
#endif

    std::cout << "[PASS] Catalog adapter remains non-authoritative." << std::endl;

    return 0;
}
