#pragma once

#include <JuceHeader.h>

#include "OrchConductorRuntimePresetCatalog.h"

struct OrchConductorNarrativeScanSelection
{
    int combiId = -1;
    int laneIndex = -1;
    int pointIndex = -1;
    double scanPosition = 0.0;
    bool isValid = false;
};

class OrchConductorNarrativeScanResolver
{
public:
    static constexpr double defaultHysteresisMargin = 0.03;

    static OrchConductorNarrativeScanSelection resolve(
        const OrchConductorRuntimePresetCatalog& catalog,
        int laneIndex,
        double scanPosition,
        int previousPointIndex = -1,
        double hysteresisMargin = defaultHysteresisMargin) noexcept;

private:
    static double clamp01(double value) noexcept;
};
