#include "OrchConductorNarrativeScanResolver.h"

#include <cmath>

namespace
{
    int findNearestPointIndex(const OrchConductorRuntimePresetCatalog& catalog,
                              int laneIndex,
                              double scanPosition) noexcept
    {
        const int pointCount = catalog.getNarrativeLanePointCount(laneIndex);

        if (pointCount <= 0)
            return -1;

        int bestIndex = 0;
        double bestDistance = std::abs(catalog.getNarrativeLanePointPosition(laneIndex, 0) - scanPosition);

        for (int pointIndex = 1; pointIndex < pointCount; ++pointIndex)
        {
            const auto position = catalog.getNarrativeLanePointPosition(laneIndex, pointIndex);
            const auto distance = std::abs(position - scanPosition);

            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = pointIndex;
            }
        }

        return bestIndex;
    }

    int resolveWithHysteresis(const OrchConductorRuntimePresetCatalog& catalog,
                              int laneIndex,
                              double scanPosition,
                              int previousPointIndex,
                              double hysteresisMargin) noexcept
    {
        const int pointCount = catalog.getNarrativeLanePointCount(laneIndex);

        if (previousPointIndex < 0 || previousPointIndex >= pointCount)
            return findNearestPointIndex(catalog, laneIndex, scanPosition);

        const auto currentPosition =
            catalog.getNarrativeLanePointPosition(laneIndex, previousPointIndex);

        if (previousPointIndex > 0)
        {
            const auto leftPosition =
                catalog.getNarrativeLanePointPosition(laneIndex, previousPointIndex - 1);

            const auto leftBoundary = (leftPosition + currentPosition) * 0.5;

            if (scanPosition < leftBoundary - hysteresisMargin)
                return findNearestPointIndex(catalog, laneIndex, scanPosition);
        }

        if (previousPointIndex + 1 < pointCount)
        {
            const auto rightPosition =
                catalog.getNarrativeLanePointPosition(laneIndex, previousPointIndex + 1);

            const auto rightBoundary = (currentPosition + rightPosition) * 0.5;

            if (scanPosition > rightBoundary + hysteresisMargin)
                return findNearestPointIndex(catalog, laneIndex, scanPosition);
        }

        return previousPointIndex;
    }
}

double OrchConductorNarrativeScanResolver::clamp01(double value) noexcept
{
    if (value < 0.0)
        return 0.0;

    if (value > 1.0)
        return 1.0;

    return value;
}

OrchConductorNarrativeScanSelection OrchConductorNarrativeScanResolver::resolve(
    const OrchConductorRuntimePresetCatalog& catalog,
    int laneIndex,
    double scanPosition,
    int previousPointIndex,
    double hysteresisMargin) noexcept
{
    OrchConductorNarrativeScanSelection selection;
    selection.laneIndex = laneIndex;
    selection.scanPosition = clamp01(scanPosition);

    if (laneIndex < 0 || laneIndex >= catalog.getNarrativeLaneCount())
        return selection;

    const int pointCount = catalog.getNarrativeLanePointCount(laneIndex);

    if (pointCount <= 0)
        return selection;

    const auto safeHysteresisMargin = juce::jlimit(0.0, 0.49, hysteresisMargin);

    const int selectedPointIndex = resolveWithHysteresis(catalog,
                                                         laneIndex,
                                                         selection.scanPosition,
                                                         previousPointIndex,
                                                         safeHysteresisMargin);

    if (selectedPointIndex < 0 || selectedPointIndex >= pointCount)
        return selection;

    const int combiId = catalog.getNarrativeLanePointCombiId(laneIndex, selectedPointIndex);

    // A lane point may reference a user combi (id >= the catalog's factory
    // combi count) - user combis live on the processor, not the runtime
    // catalog, so only bound-check against the full combi-id range (0..127).
    // The processor's send path resolves an unknown id to silence, not a crash.
    if (combiId < 0 || combiId > 127)
        return selection;

    selection.combiId = combiId;
    selection.pointIndex = selectedPointIndex;
    selection.isValid = true;

    return selection;
}
