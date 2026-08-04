#include "OrchConductorRuntimePresetCatalog.h"

namespace
{

constexpr int expectedFactorySectionCount = 5;
constexpr int expectedFactoryWoodwindPresetCount = 20;
constexpr int expectedFactoryBrassPresetCount = 17;
constexpr int expectedFactoryPercussionPresetCount = 9;
constexpr int expectedFactoryStringPresetCount = 14;
constexpr int expectedFactoryCombiPresetCount = 29;

orchconductor::SectionPresetDefinition makeFallbackSectionPreset(const juce::String& section,
                                                                  int factoryId)
{
    orchconductor::SectionPresetDefinition preset;
    preset.id = section + "-" + juce::String(factoryId);
    preset.name = section + " fallback preset " + juce::String(factoryId);
    preset.section = section;
    preset.factoryId = factoryId;
    return preset;
}

orchconductor::CombiPresetDefinition makeFallbackCombiPreset(int factoryId)
{
    orchconductor::CombiPresetDefinition preset;
    preset.id = "combi-" + juce::String(factoryId);
    preset.name = "Combi fallback preset " + juce::String(factoryId);
    preset.category = "Combi";
    preset.description = "Fallback factory-shape metadata placeholder.";
    preset.factoryId = factoryId;
    return preset;
}

void addFallbackSectionPresets(std::vector<orchconductor::SectionPresetDefinition>& presets,
                               const juce::String& section,
                               int count)
{
    presets.reserve(static_cast<size_t>(count));

    for (int index = 0; index < count; ++index)
        presets.push_back(makeFallbackSectionPreset(section, index));
}

void addFallbackCombiPresets(std::vector<orchconductor::CombiPresetDefinition>& presets,
                             int count)
{
    presets.reserve(static_cast<size_t>(count));

    for (int index = 0; index < count; ++index)
        presets.push_back(makeFallbackCombiPreset(index));
}

orchconductor::PresetLibraryDefinition createFallbackFactoryShapeLibrary()
{
    orchconductor::PresetLibraryDefinition library;

    library.libraryName = "OrchConductor fallback factory shape";
    library.libraryVersion = "fallback-shape";
    library.pluginTarget = "OrchConductor";
    library.phase = "Phase 4M";

    addFallbackSectionPresets(library.woodwindPresets, "woodwinds", expectedFactoryWoodwindPresetCount);
    addFallbackSectionPresets(library.brassPresets, "brass", expectedFactoryBrassPresetCount);
    addFallbackSectionPresets(library.percussionPresets, "percussion", expectedFactoryPercussionPresetCount);
    addFallbackSectionPresets(library.stringPresets, "strings", expectedFactoryStringPresetCount);
    addFallbackCombiPresets(library.combiPresets, expectedFactoryCombiPresetCount);

    return library;
}

bool hasExpectedSectionPresetCounts(const orchconductor::PresetLibraryDefinition& library) noexcept
{
    return static_cast<int>(library.woodwindPresets.size()) == expectedFactoryWoodwindPresetCount
        && static_cast<int>(library.brassPresets.size()) == expectedFactoryBrassPresetCount
        && static_cast<int>(library.percussionPresets.size()) == expectedFactoryPercussionPresetCount
        && static_cast<int>(library.stringPresets.size()) == expectedFactoryStringPresetCount;
}

const std::vector<orchconductor::SectionPresetDefinition>* findSectionPresets(
    const orchconductor::PresetLibraryDefinition& library,
    const juce::String& sectionId) noexcept
{
    if (sectionId == "woodwinds")
        return &library.woodwindPresets;

    if (sectionId == "brass")
        return &library.brassPresets;

    if (sectionId == "percussion")
        return &library.percussionPresets;

    if (sectionId == "strings")
        return &library.stringPresets;

    return nullptr;
}

} // namespace

OrchConductorRuntimePresetCatalog OrchConductorRuntimePresetCatalog::createFallbackCatalog()
{
    return OrchConductorRuntimePresetCatalog(
        true,
        true,
        "Runtime preset catalog using fallback factory-shape label metadata.",
        createFallbackFactoryShapeLibrary());
}

OrchConductorRuntimePresetCatalog OrchConductorRuntimePresetCatalog::createFromRuntimeSource(
    const orchconductor::RuntimePresetSourceResult& source)
{
    const auto sourceDiagnostic = source.diagnosticMessage;

    if (source.wasLoaded() && ! source.requiresHardcodedFallback())
    {
        return OrchConductorRuntimePresetCatalog(
            true,
            false,
            sourceDiagnostic.isNotEmpty()
                ? "Runtime preset catalog ready from runtime source: " + sourceDiagnostic
                : "Runtime preset catalog ready from runtime source.",
            source.library);
    }

    return OrchConductorRuntimePresetCatalog(
        true,
        true,
        sourceDiagnostic.isNotEmpty()
            ? "Runtime preset catalog using fallback factory-shape label metadata because runtime source is unavailable: " + sourceDiagnostic
            : "Runtime preset catalog using fallback factory-shape label metadata because runtime source is unavailable.",
        createFallbackFactoryShapeLibrary());
}

bool OrchConductorRuntimePresetCatalog::isReady() const noexcept
{
    return ready_;
}

bool OrchConductorRuntimePresetCatalog::requiresFallback() const noexcept
{
    return fallbackRequired_;
}

juce::String OrchConductorRuntimePresetCatalog::getDiagnosticMessage() const
{
    return diagnosticMessage_;
}

int OrchConductorRuntimePresetCatalog::getSectionCount() const noexcept
{
    int count = 0;

    if (! library_.woodwindPresets.empty())
        ++count;

    if (! library_.brassPresets.empty())
        ++count;

    if (! library_.percussionPresets.empty())
        ++count;

    if (! library_.stringPresets.empty())
        ++count;

    if (! library_.combiPresets.empty())
        ++count;

    return count;
}

int OrchConductorRuntimePresetCatalog::getSectionPresetCount(const juce::String& sectionId) const noexcept
{
    if (const auto* presets = findSectionPresets(library_, sectionId))
        return static_cast<int>(presets->size());

    return 0;
}

int OrchConductorRuntimePresetCatalog::getCombiPresetCount() const noexcept
{
    return static_cast<int>(library_.combiPresets.size());
}

bool OrchConductorRuntimePresetCatalog::hasExpectedFactoryShape() const noexcept
{
    return library_.isValid()
        && getSectionCount() == expectedFactorySectionCount
        && hasExpectedSectionPresetCounts(library_)
        && getCombiPresetCount() == expectedFactoryCombiPresetCount;
}

juce::String OrchConductorRuntimePresetCatalog::getSectionPresetLabel(const juce::String& sectionId,
                                                                       int presetIndex) const
{
    if (presetIndex < 0)
        return {};

    const auto* presets = findSectionPresets(library_, sectionId);

    if (presets == nullptr)
        return {};

    const auto index = static_cast<size_t>(presetIndex);

    if (index >= presets->size())
        return {};

    return (*presets)[index].name;
}

juce::String OrchConductorRuntimePresetCatalog::getCombiPresetLabel(int presetIndex) const
{
    if (presetIndex < 0)
        return {};

    const auto index = static_cast<size_t>(presetIndex);

    if (index >= library_.combiPresets.size())
        return {};

    return library_.combiPresets[index].name;
}

OrchConductorRuntimePresetCatalog::OrchConductorRuntimePresetCatalog(
    bool ready,
    bool fallbackRequired,
    juce::String diagnosticMessage,
    orchconductor::PresetLibraryDefinition library)
    : ready_(ready),
      fallbackRequired_(fallbackRequired),
      diagnosticMessage_(std::move(diagnosticMessage)),
      library_(std::move(library))
{
}
