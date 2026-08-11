#include <JuceHeader.h>

#include "OrchConductorEmbeddedFactoryJson.h"
#include "OrchConductorNarrativeScanResolver.h"
#include "OrchConductorRuntimePresetCatalog.h"
#include "OrchConductorRuntimePresetSource.h"

namespace
{
    int findLaneIndex(const OrchConductorRuntimePresetCatalog& catalog,
                      const juce::String& laneId)
    {
        for (int laneIndex = 0; laneIndex < catalog.getNarrativeLaneCount(); ++laneIndex)
        {
            if (catalog.getNarrativeLaneId(laneIndex) == laneId)
                return laneIndex;
        }

        return -1;
    }

    bool expectSelection(const OrchConductorRuntimePresetCatalog& catalog,
                         const juce::String& laneId,
                         double scanPosition,
                         int previousPointIndex,
                         int expectedPointIndex,
                         int expectedCombiId)
    {
        const int laneIndex = findLaneIndex(catalog, laneId);

        if (laneIndex < 0)
            return false;

        const auto selection =
            OrchConductorNarrativeScanResolver::resolve(catalog,
                                                        laneIndex,
                                                        scanPosition,
                                                        previousPointIndex);

        return selection.isValid
            && selection.laneIndex == laneIndex
            && selection.pointIndex == expectedPointIndex
            && selection.combiId == expectedCombiId;
    }

    bool runNarrativeScanResolverCheck()
    {
        const auto runtimeSource =
            orchconductor::RuntimePresetSource::loadEmbeddedFactoryJsonIfEnabled();

        if (! runtimeSource.wasLoaded() || runtimeSource.requiresHardcodedFallback())
            return false;

        const auto catalog =
            OrchConductorRuntimePresetCatalog::createFromRuntimeSource(runtimeSource);

        if (! catalog.isReady() || catalog.requiresFallback() || ! catalog.hasExpectedFactoryShape())
            return false;

        if (! expectSelection(catalog, "organic_build", 0.0, -1, 0, 28))
            return false;

        if (! expectSelection(catalog, "organic_build", 1.0, -1, 5, 2))
            return false;

        if (! expectSelection(catalog, "anticlimax", 1.0, -1, 5, 1))
            return false;

        if (! expectSelection(catalog, "bright_build", 0.0, -1, 0, 14))
            return false;

        // organic_build points 1 and 2 are expected at 0.18 and 0.35.
        // Their midpoint is 0.265. With default hysteresis 0.03,
        // previous point 1 should remain sticky until scan > 0.295.
        if (! expectSelection(catalog, "organic_build", 0.28, 1, 1, 26))
            return false;

        if (! expectSelection(catalog, "organic_build", 0.31, 1, 2, 4))
            return false;

        return true;
    }
}

int main()
{
    const bool passed = runNarrativeScanResolverCheck();

    if (! passed)
    {
        std::cerr << "OrchConductorNarrativeScanResolverCheck failed." << std::endl;
        return 1;
    }

    std::cout << "OrchConductorNarrativeScanResolverCheck passed." << std::endl;
    return 0;
}
