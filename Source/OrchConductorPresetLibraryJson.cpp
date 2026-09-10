#include "OrchConductorPresetLibraryJson.h"

namespace orchconductor
{
namespace
{

juce::String makeError(const juce::String& message)
{
    return "Preset library JSON load failed: " + message;
}

const juce::DynamicObject* asObject(const juce::var& value)
{
    return value.getDynamicObject();
}

const juce::Array<juce::var>* asArray(const juce::var& value)
{
    return value.getArray();
}

bool hasProperty(const juce::DynamicObject& object, const juce::Identifier& propertyName)
{
    return object.hasProperty(propertyName);
}

juce::String getString(const juce::DynamicObject& object,
                       const juce::Identifier& propertyName,
                       const juce::String& defaultValue = {})
{
    if (! hasProperty(object, propertyName))
        return defaultValue;

    return object.getProperty(propertyName).toString();
}

int getInt(const juce::DynamicObject& object,
           const juce::Identifier& propertyName,
           int defaultValue = 0)
{
    if (! hasProperty(object, propertyName))
        return defaultValue;

    return static_cast<int>(object.getProperty(propertyName));
}

bool getBool(const juce::DynamicObject& object,
             const juce::Identifier& propertyName,
             bool defaultValue = false)
{
    if (! hasProperty(object, propertyName))
        return defaultValue;

    return static_cast<bool>(object.getProperty(propertyName));
}

double getDouble(const juce::DynamicObject& object,
                 const juce::Identifier& propertyName,
                 double defaultValue = 0.0)
{
    if (! hasProperty(object, propertyName))
        return defaultValue;

    return static_cast<double>(object.getProperty(propertyName));
}

double clamp01(double value) noexcept
{
    if (value < 0.0)
        return 0.0;

    if (value > 1.0)
        return 1.0;

    return value;
}

void readNarrativeMetadata(const juce::DynamicObject& presetObject,
                           NarrativeMetadata& destination)
{
    const auto metadata = presetObject.getProperty("metadata");
    const auto* metadataObject = asObject(metadata);

    if (metadataObject == nullptr)
        return;

    destination.energy = clamp01(getDouble(*metadataObject, "energy", destination.energy));
    destination.density = clamp01(getDouble(*metadataObject, "density", destination.density));
    destination.brightness = clamp01(getDouble(*metadataObject, "brightness", destination.brightness));
    destination.weight = clamp01(getDouble(*metadataObject, "weight", destination.weight));
    destination.tension = clamp01(getDouble(*metadataObject, "tension", destination.tension));

    const auto registerName = getString(*metadataObject, "register", destination.registerName).trim();
    const auto role = getString(*metadataObject, "role", destination.role).trim();
    const auto transitionBehavior = getString(*metadataObject,
                                              "transition_behavior",
                                              destination.transitionBehavior).trim();
    const auto narrativeLane = getString(*metadataObject, "narrative_lane", destination.narrativeLane).trim();

    if (registerName.isNotEmpty())
        destination.registerName = registerName;

    if (role.isNotEmpty())
        destination.role = role;

    if (transitionBehavior.isNotEmpty())
        destination.transitionBehavior = transitionBehavior;

    destination.narrativeLane = narrativeLane;
}

bool readPresetValues(const juce::DynamicObject& presetObject,
                      std::vector<PresetValue>& destination,
                      juce::String& errorMessage,
                      const juce::String& context)
{
    destination.clear();

    const auto values = presetObject.getProperty("values");
    const auto* valuesArray = asArray(values);

    if (valuesArray == nullptr)
    {
        errorMessage = makeError(context + " is missing a values array.");
        return false;
    }

    for (int i = 0; i < valuesArray->size(); ++i)
    {
        const auto* valueObject = asObject(valuesArray->getReference(i));

        if (valueObject == nullptr)
        {
            errorMessage = makeError(context + " contains a non-object value entry at index "
                                     + juce::String(i) + ".");
            return false;
        }

        PresetValue presetValue;
        presetValue.ccNumber = getInt(*valueObject, "cc", 0);
        presetValue.value = getInt(*valueObject, "value", 0);

        if (! presetValue.isValid())
        {
            errorMessage = makeError(context + " contains an invalid value entry at index "
                                     + juce::String(i) + ".");
            return false;
        }

        destination.push_back(presetValue);
    }

    return true;
}

bool readSectionPresetArray(const juce::DynamicObject& sectionPresetsObject,
                            const juce::Identifier& propertyName,
                            std::vector<SectionPresetDefinition>& destination,
                            juce::String& errorMessage)
{
    destination.clear();

    const auto presets = sectionPresetsObject.getProperty(propertyName);
    const auto* presetsArray = asArray(presets);

    if (presetsArray == nullptr)
    {
        errorMessage = makeError("sectionPresets." + propertyName.toString()
                                 + " is missing or is not an array.");
        return false;
    }

    for (int i = 0; i < presetsArray->size(); ++i)
    {
        const auto* presetObject = asObject(presetsArray->getReference(i));

        if (presetObject == nullptr)
        {
            errorMessage = makeError("sectionPresets." + propertyName.toString()
                                     + " contains a non-object preset at index "
                                     + juce::String(i) + ".");
            return false;
        }

        SectionPresetDefinition preset;
        preset.id = getString(*presetObject, "id");
        preset.name = getString(*presetObject, "name");
        preset.section = getString(*presetObject, "section");
        preset.factoryId = getInt(*presetObject, "factoryId", 0);

                readNarrativeMetadata(*presetObject, preset.metadata);
const auto context = "sectionPresets." + propertyName.toString()
                           + "[" + juce::String(i) + "]";

        if (! readPresetValues(*presetObject, preset.values, errorMessage, context))
            return false;

        if (! preset.isValid())
        {
            errorMessage = makeError(context + " is invalid.");
            return false;
        }

        destination.push_back(preset);
    }

    return true;
}

bool readCombiPresets(const juce::DynamicObject& rootObject,
                      std::vector<CombiPresetDefinition>& destination,
                      juce::String& errorMessage)
{
    destination.clear();

    const auto combiPresets = rootObject.getProperty("combiPresets");
    const auto* combiPresetsArray = asArray(combiPresets);

    if (combiPresetsArray == nullptr)
    {
        errorMessage = makeError("combiPresets is missing or is not an array.");
        return false;
    }

    for (int i = 0; i < combiPresetsArray->size(); ++i)
    {
        const auto* presetObject = asObject(combiPresetsArray->getReference(i));

        if (presetObject == nullptr)
        {
            errorMessage = makeError("combiPresets contains a non-object preset at index "
                                     + juce::String(i) + ".");
            return false;
        }

        CombiPresetDefinition preset;
        preset.id = getString(*presetObject, "id");
        preset.name = getString(*presetObject, "name");
        preset.category = getString(*presetObject, "category");
        preset.description = getString(*presetObject, "description");
        preset.factoryId = getInt(*presetObject, "factoryId", 0);

                readNarrativeMetadata(*presetObject, preset.metadata);
const auto context = "combiPresets[" + juce::String(i) + "]";

        if (! readPresetValues(*presetObject, preset.values, errorMessage, context))
            return false;

        if (! preset.isValid())
        {
            errorMessage = makeError(context + " is invalid.");
            return false;
        }

        destination.push_back(preset);
    }

    return true;
}

bool readNarrativeLanes(const juce::DynamicObject& rootObject,
                        std::vector<NarrativeLaneDefinition>& destination,
                        juce::String& errorMessage)
{
    destination.clear();

    const auto narrativeLanes = rootObject.getProperty("narrativeLanes");

    if (narrativeLanes.isVoid())
        return true;

    const auto* narrativeLanesArray = asArray(narrativeLanes);

    if (narrativeLanesArray == nullptr)
    {
        errorMessage = makeError("narrativeLanes is present but is not an array.");
        return false;
    }

    for (int laneIndex = 0; laneIndex < narrativeLanesArray->size(); ++laneIndex)
    {
        const auto* laneObject = asObject(narrativeLanesArray->getReference(laneIndex));

        if (laneObject == nullptr)
        {
            errorMessage = makeError("narrativeLanes contains a non-object lane at index "
                                     + juce::String(laneIndex) + ".");
            return false;
        }

        NarrativeLaneDefinition lane;
        lane.id = getString(*laneObject, "id").trim();
        lane.name = getString(*laneObject, "name").trim();
        lane.description = getString(*laneObject, "description").trim();

        const auto points = laneObject->getProperty("points");
        const auto* pointsArray = asArray(points);

        if (pointsArray == nullptr)
        {
            errorMessage = makeError("narrativeLanes[" + juce::String(laneIndex)
                                     + "] is missing a points array.");
            return false;
        }

        for (int pointIndex = 0; pointIndex < pointsArray->size(); ++pointIndex)
        {
            const auto* pointObject = asObject(pointsArray->getReference(pointIndex));

            if (pointObject == nullptr)
            {
                errorMessage = makeError("narrativeLanes[" + juce::String(laneIndex)
                                         + "].points contains a non-object point at index "
                                         + juce::String(pointIndex) + ".");
                return false;
            }

            NarrativeLanePointDefinition point;
            point.position = getDouble(*pointObject, "position", 0.0);
            point.combiId = getInt(*pointObject, "combiId", 0);
            point.label = getString(*pointObject, "label").trim();
            point.pitchFieldIndex = getInt(*pointObject, "pitchFieldIndex", -1);
            point.harpValue = getInt(*pointObject, "harpValue", -1);
            point.pianoValue = getInt(*pointObject, "pianoValue", -1);
            point.gateResponseMode = getInt(*pointObject, "gateResponseMode", -1);
            point.gateResponseAmount = getInt(*pointObject, "gateResponseAmount", -1);

            const auto transitionTags = pointObject->getProperty("transitionTags");
            const auto* transitionTagsArray = asArray(transitionTags);

            if (transitionTagsArray != nullptr)
            {
                for (const auto& tagValue : *transitionTagsArray)
                {
                    const auto tag = tagValue.toString().trim();

                    if (tag.isNotEmpty())
                        point.transitionTags.add(tag);
                }
            }

            if (! point.isValid())
            {
                errorMessage = makeError("narrativeLanes[" + juce::String(laneIndex)
                                         + "].points[" + juce::String(pointIndex)
                                         + "] is invalid.");
                return false;
            }

            lane.points.push_back(point);
        }

        if (! lane.isValid())
        {
            errorMessage = makeError("narrativeLanes[" + juce::String(laneIndex)
                                     + "] is invalid.");
            return false;
        }

        destination.push_back(lane);
    }

    return true;
}

bool readMidi(const juce::DynamicObject& rootObject,
              MidiLibraryMetadata& destination,
              juce::String& errorMessage)
{
    const auto midi = rootObject.getProperty("midi");
    const auto* midiObject = asObject(midi);

    if (midiObject == nullptr)
    {
        errorMessage = makeError("midi is missing or is not an object.");
        return false;
    }

    destination.channel = getInt(*midiObject, "channel", 1);

    const auto ccRange = midiObject->getProperty("ccRange");
    const auto* ccRangeObject = asObject(ccRange);

    if (ccRangeObject == nullptr)
    {
        errorMessage = makeError("midi.ccRange is missing or is not an object.");
        return false;
    }

    destination.ccMin = getInt(*ccRangeObject, "min", 20);
    destination.ccMax = getInt(*ccRangeObject, "max", 54);

    destination.reservedControllers.clear();

    const auto reservedControllers = midiObject->getProperty("reservedControllers");
    const auto* reservedControllersArray = asArray(reservedControllers);

    if (reservedControllersArray != nullptr)
    {
        for (int i = 0; i < reservedControllersArray->size(); ++i)
        {
            const auto* reservedObject = asObject(reservedControllersArray->getReference(i));

            if (reservedObject == nullptr)
            {
                errorMessage = makeError("midi.reservedControllers contains a non-object entry at index "
                                         + juce::String(i) + ".");
                return false;
            }

            ReservedControllerDefinition reservedController;
            reservedController.ccNumber = getInt(*reservedObject, "cc", 0);
            reservedController.name = getString(*reservedObject, "name");
            reservedController.status = getString(*reservedObject, "status");
            reservedController.defaultValue = getInt(*reservedObject, "defaultValue", 0);

            if (! reservedController.isValid())
            {
                errorMessage = makeError("midi.reservedControllers contains an invalid entry at index "
                                         + juce::String(i) + ".");
                return false;
            }

            destination.reservedControllers.push_back(reservedController);
        }
    }

    if (! destination.isValid())
    {
        errorMessage = makeError("midi metadata is invalid.");
        return false;
    }

    return true;
}

bool readInstruments(const juce::DynamicObject& rootObject,
                     std::vector<InstrumentDefinition>& destination,
                     juce::String& errorMessage)
{
    destination.clear();

    const auto instruments = rootObject.getProperty("instruments");
    const auto* instrumentsArray = asArray(instruments);

    if (instrumentsArray == nullptr)
    {
        errorMessage = makeError("instruments is missing or is not an array.");
        return false;
    }

    for (int i = 0; i < instrumentsArray->size(); ++i)
    {
        const auto* instrumentObject = asObject(instrumentsArray->getReference(i));

        if (instrumentObject == nullptr)
        {
            errorMessage = makeError("instruments contains a non-object entry at index "
                                     + juce::String(i) + ".");
            return false;
        }

        InstrumentDefinition instrument;
        instrument.id = getString(*instrumentObject, "id");
        instrument.name = getString(*instrumentObject, "name");
        instrument.section = getString(*instrumentObject, "section");
        instrument.ccNumber = getInt(*instrumentObject, "cc", 0);
        instrument.reserved = getBool(*instrumentObject, "reserved", false);

        if (! instrument.isValid())
        {
            errorMessage = makeError("instruments contains an invalid entry at index "
                                     + juce::String(i) + ".");
            return false;
        }

        destination.push_back(instrument);
    }

    return true;
}

bool readPlayerProfile(const juce::DynamicObject& rootObject,
                       PlayerProfile& destination,
                       juce::String& errorMessage)
{
    const auto playerProfile = rootObject.getProperty("playerProfile");
    const auto* playerProfileObject = asObject(playerProfile);

    if (playerProfileObject == nullptr)
    {
        errorMessage = makeError("playerProfile is missing or is not an object.");
        return false;
    }

    destination.defaultMaxPlayers = getInt(*playerProfileObject, "defaultMaxPlayers", 1);
    destination.reservedMaxPlayers = getInt(*playerProfileObject, "reservedMaxPlayers", 0);
    destination.overrides.clear();

    const auto overrides = playerProfileObject->getProperty("overrides");
    const auto* overridesArray = asArray(overrides);

    if (overridesArray != nullptr)
    {
        for (int i = 0; i < overridesArray->size(); ++i)
        {
            const auto* overrideObject = asObject(overridesArray->getReference(i));

            if (overrideObject == nullptr)
            {
                errorMessage = makeError("playerProfile.overrides contains a non-object entry at index "
                                         + juce::String(i) + ".");
                return false;
            }

            PlayerProfileOverride overrideEntry;
            overrideEntry.ccNumber = getInt(*overrideObject, "cc", 0);
            overrideEntry.maxPlayers = getInt(*overrideObject, "maxPlayers", 1);

            if (! overrideEntry.isValid())
            {
                errorMessage = makeError("playerProfile.overrides contains an invalid entry at index "
                                         + juce::String(i) + ".");
                return false;
            }

            destination.overrides.push_back(overrideEntry);
        }
    }

    if (! destination.isValid())
    {
        errorMessage = makeError("playerProfile is invalid.");
        return false;
    }

    return true;
}

bool readSectionPresets(const juce::DynamicObject& rootObject,
                        PresetLibraryDefinition& destination,
                        juce::String& errorMessage)
{
    const auto sectionPresets = rootObject.getProperty("sectionPresets");
    const auto* sectionPresetsObject = asObject(sectionPresets);

    if (sectionPresetsObject == nullptr)
    {
        errorMessage = makeError("sectionPresets is missing or is not an object.");
        return false;
    }

    return readSectionPresetArray(*sectionPresetsObject, "woodwinds", destination.woodwindPresets, errorMessage)
        && readSectionPresetArray(*sectionPresetsObject, "brass", destination.brassPresets, errorMessage)
        && readSectionPresetArray(*sectionPresetsObject, "percussion", destination.percussionPresets, errorMessage)
        && readSectionPresetArray(*sectionPresetsObject, "strings", destination.stringPresets, errorMessage);
}

} // namespace

PresetLibraryJsonLoadResult PresetLibraryJsonLoader::fromJsonText(const juce::String& jsonText)
{
    PresetLibraryJsonLoadResult result;

    auto textToParse = jsonText;

    if (textToParse.isNotEmpty() && textToParse[0] == 0xfeff)
        textToParse = textToParse.substring(1);

    auto parsed = juce::JSON::parse(textToParse);
    const auto* rootObject = asObject(parsed);

    if (rootObject == nullptr)
    {
        result.errorMessage = makeError("root value is not a JSON object.");
        return result;
    }

    result.library.schema = getString(*rootObject, "schema");
    result.library.schemaVersion = getInt(*rootObject, "schemaVersion", 0);
    result.library.libraryName = getString(*rootObject, "libraryName");
    result.library.libraryVersion = getString(*rootObject, "libraryVersion");
    result.library.pluginTarget = getString(*rootObject, "pluginTarget");
    result.library.phase = getString(*rootObject, "phase");

    if (! readMidi(*rootObject, result.library.midi, result.errorMessage))
        return result;

    if (! readInstruments(*rootObject, result.library.instruments, result.errorMessage))
        return result;

    if (! readPlayerProfile(*rootObject, result.library.playerProfile, result.errorMessage))
        return result;

    if (! readSectionPresets(*rootObject, result.library, result.errorMessage))
        return result;

    if (! readCombiPresets(*rootObject, result.library.combiPresets, result.errorMessage))
        return result;

    if (! readNarrativeLanes(*rootObject, result.library.narrativeLanes, result.errorMessage))
        return result;

    if (! result.library.isValid())
    {
        result.errorMessage = makeError("loaded library failed final model validation.");
        return result;
    }

    return result;
}

PresetLibraryJsonLoadResult PresetLibraryJsonLoader::fromJsonFile(const juce::File& file)
{
    PresetLibraryJsonLoadResult result;

    if (! file.existsAsFile())
    {
        result.errorMessage = makeError("file does not exist: " + file.getFullPathName());
        return result;
    }

    return fromJsonText(file.loadFileAsString());
}

} // namespace orchconductor
