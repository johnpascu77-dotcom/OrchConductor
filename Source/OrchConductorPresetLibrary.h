#pragma once

#include <JuceHeader.h>
#include <vector>

namespace orchconductor
{

struct PresetValue
{
    int ccNumber = 0;
    int value = 0;

    bool isValid() const noexcept
    {
        return ccNumber >= 0
            && ccNumber <= 127
            && value >= 0
            && value <= 127;
    }
};

struct InstrumentDefinition
{
    juce::String id;
    juce::String name;
    juce::String section;
    int ccNumber = 0;
    bool reserved = false;

    bool isValid() const noexcept
    {
        return id.isNotEmpty()
            && name.isNotEmpty()
            && section.isNotEmpty()
            && ccNumber >= 0
            && ccNumber <= 127;
    }
};

struct PlayerProfileOverride
{
    int ccNumber = 0;
    int maxPlayers = 1;

    bool isValid() const noexcept
    {
        return ccNumber >= 0
            && ccNumber <= 127
            && maxPlayers >= 0;
    }
};

struct PlayerProfile
{
    int defaultMaxPlayers = 1;
    int reservedMaxPlayers = 0;
    std::vector<PlayerProfileOverride> overrides;

    bool isValid() const noexcept
    {
        if (defaultMaxPlayers < 0 || reservedMaxPlayers < 0)
            return false;

        for (const auto& overrideEntry : overrides)
        {
            if (! overrideEntry.isValid())
                return false;
        }

        return true;
    }
};

struct NarrativeMetadata
{
    double energy = 0.0;
    double density = 0.0;
    double brightness = 0.0;
    double weight = 0.0;
    double tension = 0.0;

    juce::String registerName { "mixed" };
    juce::String role { "utility" };
    juce::String transitionBehavior { "neutral" };
    juce::String narrativeLane;

    bool isValid() const noexcept
    {
        return energy >= 0.0 && energy <= 1.0
            && density >= 0.0 && density <= 1.0
            && brightness >= 0.0 && brightness <= 1.0
            && weight >= 0.0 && weight <= 1.0
            && tension >= 0.0 && tension <= 1.0
            && registerName.isNotEmpty()
            && role.isNotEmpty()
            && transitionBehavior.isNotEmpty();
    }
};
struct NarrativeLanePointDefinition
{
    double position = 0.0;
    int combiId = 0;
    juce::String label;
    juce::StringArray transitionTags;

    // Optional OrchNoteFilter pitch-class field for this lane point. -1 = leave
    // the harmonic field alone; >= 0 is an index into OrchNoteFilter's
    // append-only field-preset list (0 = Chromatic). Sent as a CC when the
    // resolved lane point changes - see OrchConductorProcessor's field-select CC.
    int pitchFieldIndex = -1;

    // Optional Harp (CC49) and Piano (CC55) values for this lane point. -1 =
    // leave them at the combi's own value (which, for every factory combi, is
    // 0). >= 0 overrides that value when this point is the resolved one, the
    // same way a user combi's harpValue/pianoValue works. This is the only way
    // a Narrative Scan run can bring the Harp or Piano in, since no factory
    // combi touches CC49/CC55.
    int harpValue = -1;
    int pianoValue = -1;

    bool isValid() const noexcept
    {
        return position >= 0.0
            && position <= 1.0
            && combiId >= 0
            && pitchFieldIndex >= -1
            && harpValue >= -1
            && harpValue <= 127
            && pianoValue >= -1
            && pianoValue <= 127;
    }
};

struct NarrativeLaneDefinition
{
    juce::String id;
    juce::String name;
    juce::String description;
    std::vector<NarrativeLanePointDefinition> points;

    bool hasValidSortedPoints() const noexcept
    {
        if (points.empty())
            return false;

        double previousPosition = -1.0;

        for (const auto& point : points)
        {
            if (! point.isValid())
                return false;

            if (point.position <= previousPosition)
                return false;

            previousPosition = point.position;
        }

        return true;
    }

    bool isValid() const noexcept
    {
        return id.isNotEmpty()
            && name.isNotEmpty()
            && hasValidSortedPoints();
    }
};
struct SectionPresetDefinition
{
    juce::String id;
    juce::String name;
    juce::String section;
    int factoryId = 0;
    std::vector<PresetValue> values;

        NarrativeMetadata metadata;
bool isValid() const noexcept
    {
        if (id.isEmpty()
            || name.isEmpty()
            || section.isEmpty()
            || factoryId < 0
            || ! metadata.isValid())
        {
            return false;
        }

        for (const auto& presetValue : values)
        {
            if (! presetValue.isValid())
                return false;
        }

        return true;
    }
};

struct CombiPresetDefinition
{
    juce::String id;
    juce::String name;
    juce::String category;
    juce::String description;
    int factoryId = 0;
    std::vector<PresetValue> values;

        NarrativeMetadata metadata;
bool isValid() const noexcept
    {
        if (id.isEmpty()
            || name.isEmpty()
            || category.isEmpty()
            || factoryId < 0
            || ! metadata.isValid())
        {
            return false;
        }

        for (const auto& presetValue : values)
        {
            if (! presetValue.isValid())
                return false;
        }

        return true;
    }
};

struct ReservedControllerDefinition
{
    int ccNumber = 0;
    juce::String name;
    juce::String status;
    int defaultValue = 0;

    bool isValid() const noexcept
    {
        return ccNumber >= 0
            && ccNumber <= 127
            && name.isNotEmpty()
            && status.isNotEmpty()
            && defaultValue >= 0
            && defaultValue <= 127;
    }
};

struct MidiLibraryMetadata
{
    int channel = 1;
    int ccMin = 20;
    int ccMax = 54;
    std::vector<ReservedControllerDefinition> reservedControllers;

    bool isValid() const noexcept
    {
        if (channel < 1 || channel > 16)
            return false;

        if (ccMin < 0 || ccMin > 127 || ccMax < 0 || ccMax > 127 || ccMin > ccMax)
            return false;

        for (const auto& reservedController : reservedControllers)
        {
            if (! reservedController.isValid())
                return false;
        }

        return true;
    }
};

struct PresetLibraryDefinition
{
    juce::String schema = "orchconductor.library";
    int schemaVersion = 1;
    juce::String libraryName;
    juce::String libraryVersion;
    juce::String pluginTarget;
    juce::String phase;

    MidiLibraryMetadata midi;
    std::vector<InstrumentDefinition> instruments;
    PlayerProfile playerProfile;

    std::vector<SectionPresetDefinition> woodwindPresets;
    std::vector<SectionPresetDefinition> brassPresets;
    std::vector<SectionPresetDefinition> percussionPresets;
    std::vector<SectionPresetDefinition> stringPresets;
    std::vector<CombiPresetDefinition> combiPresets;
    std::vector<NarrativeLaneDefinition> narrativeLanes;
    bool isValid() const noexcept
    {
        if (schema != "orchconductor.library")
            return false;

        if (schemaVersion != 1)
            return false;

        if (! midi.isValid())
            return false;

        if (! playerProfile.isValid())
            return false;

        for (const auto& instrument : instruments)
        {
            if (! instrument.isValid())
                return false;
        }

        for (const auto& preset : woodwindPresets)
        {
            if (! preset.isValid())
                return false;
        }

        for (const auto& preset : brassPresets)
        {
            if (! preset.isValid())
                return false;
        }

        for (const auto& preset : percussionPresets)
        {
            if (! preset.isValid())
                return false;
        }

        for (const auto& preset : stringPresets)
        {
            if (! preset.isValid())
                return false;
        }

        for (const auto& preset : combiPresets)
        {
            if (! preset.isValid())
                return false;
        }

        for (const auto& lane : narrativeLanes)
        {
            if (! lane.isValid())
                return false;
        }

        return true;
    }
};

} // namespace orchconductor
