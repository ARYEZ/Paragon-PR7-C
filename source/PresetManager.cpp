#include "PresetManager.h"

#include "Parameters.h"

namespace
{
    // The factory presets: starting points (our own settings, not copies of
    // anything), each only listing what differs from the defaults. Values
    // are in each parameter's own units (Hz, dB, ms, %, choice index).
    struct FactoryPreset
    {
        const char* name;
        std::vector<std::pair<juce::String, float>> values;
    };

    // Choice indexes, in their saved order.
    constexpr float in = 0, sidechain = 2;               // filterPlacement
    constexpr float vca = 0, opto = 1, fet = 2;          // compCharacter
    constexpr float soft = 1;                            // compRatio's SOFT position
    constexpr float infinite = 30;                       // compRatio at the top
    constexpr float softLimit = 1, autoLimit = 2;        // limitMode
    constexpr float fast = 0, medium = 1;                // gateAttack

    const std::vector<FactoryPreset>& factoryPresets()
    {
        static const std::vector<FactoryPreset> presets {
            { PresetManager::defaultName, {} },
            { "Vocal - Clear Lead", { { "hpfOn", 1 }, { "hpfFreq", 90 },
                                      { "compOn", 1 }, { "compThreshold", -22 }, { "compRatio", 3 }, { "compAttack", 5 }, { "compRelease", 150 },
                                      { "compAuto", 1 }, { "compMakeUp", 4 }, { "compCharacter", vca },
                                      { "lmfFreq", 300 }, { "lmfGain", -2 }, { "hmfFreq", 3000 }, { "hmfGain", 2 },
                                      { "hfFreq", 10000 }, { "hfGain", 3 },
                                      { "satOn", 1 }, { "satDrive", 4 }, { "satHarmonics", 20 }, { "toneThd", 20 } } },
            { "Vocal - Warm Opto", { { "hpfOn", 1 }, { "hpfFreq", 80 },
                                     { "compOn", 1 }, { "compThreshold", -26 }, { "compRatio", 4 }, { "compAttack", 10 }, { "compRelease", 300 },
                                     { "compMakeUp", 5 }, { "compCharacter", opto },
                                     { "mfFreq", 500 }, { "mfGain", -1.5f }, { "hfFreq", 12000 }, { "hfGain", 2 },
                                     { "satOn", 1 }, { "satDrive", 8 }, { "satHarmonics", 0 }, { "satMix", 60 }, { "toneThd", 30 } } },
            { "Vocal - Soft Backing", { { "hpfOn", 1 }, { "hpfFreq", 120 }, { "lpfOn", 1 }, { "lpfFreq", 14000 },
                                        { "compOn", 1 }, { "compThreshold", -24 }, { "compRatio", soft }, { "compAttack", 10 }, { "compRelease", 200 },
                                        { "compAuto", 1 }, { "compMakeUp", 3 },
                                        { "hmfFreq", 2500 }, { "hmfGain", -2 }, { "hmfQ", 0.8f }, { "widthOn", 1 }, { "width", 120 } } },
            { "Kick - Punch", { { "hpfOn", 1 }, { "hpfFreq", 30 },
                                { "gateOn", 1 }, { "gateThreshold", -30 }, { "gateRange", 30 }, { "gateAttack", fast }, { "gateHold", 50 }, { "gateRelease", 150 },
                                { "compOn", 1 }, { "compThreshold", -18 }, { "compRatio", 4 }, { "compAttack", 15 }, { "compRelease", 100 },
                                { "compMakeUp", 3 }, { "compCharacter", fet },
                                { "lfShelf", 0 }, { "lfFreq", 60 }, { "lfGain", 4 }, { "lmfFreq", 350 }, { "lmfGain", -5 },
                                { "hmfFreq", 3500 }, { "hmfGain", 3 }, { "hmfQ", 1.4f } } },
            { "Snare - Crack", { { "hpfOn", 1 }, { "hpfFreq", 80 },
                                 { "gateOn", 1 }, { "gateThreshold", -35 }, { "gateRange", 20 }, { "gateAttack", fast }, { "gateHold", 50 }, { "gateRelease", 200 },
                                 { "compOn", 1 }, { "compThreshold", -20 }, { "compRatio", 4 }, { "compAttack", 3 }, { "compRelease", 120 }, { "compMakeUp", 3 },
                                 { "lmfFreq", 200 }, { "lmfGain", 2 }, { "mfFreq", 800 }, { "mfGain", -2 }, { "mfQ", 1.5f },
                                 { "hmfFreq", 5000 }, { "hmfGain", 3 }, { "hfGain", 2 },
                                 { "satOn", 1 }, { "satDrive", 8 }, { "satHarmonics", 60 } } },
            { "Toms - Tight Gate", { { "hpfOn", 1 }, { "hpfFreq", 60 },
                                     { "gateOn", 1 }, { "gateThreshold", -30 }, { "gateRange", 40 }, { "gateAttack", medium }, { "gateHold", 100 },
                                     { "gateRelease", 300 }, { "lmfFreq", 400 }, { "lmfGain", -4 } } },
            { "Drum Bus - Glue", { { "hpfOn", 1 }, { "hpfFreq", 60 }, { "filterPlacement", sidechain },
                                   { "compOn", 1 }, { "compThreshold", -20 }, { "compRatio", 2 }, { "compAttack", 30 }, { "compRelease", 300 },
                                   { "compAuto", 1 }, { "compMakeUp", 2 },
                                   { "satOn", 1 }, { "satDrive", 6 }, { "satHarmonics", 50 }, { "satMix", 70 }, { "toneThd", 30 } } },
            { "Room Mics - Crush", { { "hpfOn", 1 }, { "hpfFreq", 150 },
                                     { "compOn", 1 }, { "compThreshold", -40 }, { "compRatio", infinite }, { "compAttack", 0.5f }, { "compRelease", 80 },
                                     { "compMakeUp", 14 }, { "compCharacter", fet }, { "satOn", 1 }, { "satDrive", 8 } } },
            { "Bass - Solid", { { "hpfOn", 1 }, { "hpfFreq", 35 },
                                { "compOn", 1 }, { "compThreshold", -24 }, { "compRatio", 4 }, { "compAttack", 10 }, { "compRelease", 200 },
                                { "compMakeUp", 4 }, { "compCharacter", opto },
                                { "lfFreq", 80 }, { "lfGain", 2 }, { "lmfFreq", 250 }, { "lmfGain", -3 }, { "hmfFreq", 1200 }, { "hmfGain", 2 },
                                { "satOn", 1 }, { "satDrive", 10 }, { "satHarmonics", 20 }, { "monoOn", 1 }, { "monoFreq", 150 } } },
            { "Acoustic Guitar - Sparkle", { { "hpfOn", 1 }, { "hpfFreq", 100 },
                                             { "compOn", 1 }, { "compThreshold", -20 }, { "compRatio", 2 }, { "compAttack", 10 }, { "compRelease", 150 },
                                             { "compMakeUp", 2 }, { "lmfFreq", 250 }, { "lmfGain", -3 }, { "hmfFreq", 4500 }, { "hmfGain", 2 },
                                             { "hmfQ", 0.8f }, { "hfFreq", 12000 }, { "hfGain", 3 }, { "widthOn", 1 }, { "width", 120 } } },
            { "Electric Guitar - Tame", { { "hpfOn", 1 }, { "hpfFreq", 90 }, { "lpfOn", 1 }, { "lpfFreq", 9000 },
                                          { "hmfFreq", 2500 }, { "hmfGain", -3 }, { "hmfQ", 2 },
                                          { "satOn", 1 }, { "satDrive", 6 }, { "satHarmonics", 40 }, { "satMix", 50 } } },
            { "Piano - Bright", { { "hpfOn", 1 }, { "hpfFreq", 40 },
                                  { "compOn", 1 }, { "compThreshold", -20 }, { "compRatio", soft }, { "compAttack", 20 }, { "compRelease", 300 },
                                  { "compAuto", 1 }, { "compMakeUp", 2 }, { "lmfFreq", 300 }, { "lmfGain", -2 }, { "hfFreq", 8000 }, { "hfGain", 2 },
                                  { "widthOn", 1 }, { "width", 115 } } },
            { "Mix Bus - Console Glue", { { "hpfOn", 1 }, { "hpfFreq", 25 }, { "filterPlacement", sidechain },
                                          { "compOn", 1 }, { "compThreshold", -16 }, { "compRatio", 1.5f }, { "compAttack", 30 }, { "compRelease", 300 },
                                          { "compAuto", 1 }, { "compMakeUp", 1.5f },
                                          { "satOn", 1 }, { "satDrive", 3 }, { "satHarmonics", 30 }, { "satMix", 50 }, { "toneThd", 40 },
                                          { "monoOn", 1 }, { "monoFreq", 100 }, { "limitOn", 1 }, { "limitThreshold", -1 }, { "limitMode", softLimit } } },
            { "Master - Loud", { { "compOn", 1 }, { "compThreshold", -18 }, { "compRatio", soft }, { "compAttack", 30 }, { "compRelease", 300 },
                                 { "compAuto", 1 }, { "compMakeUp", 2 },
                                 { "limitOn", 1 }, { "limitThreshold", -1 }, { "limitMakeUp", 4 }, { "limitMode", autoLimit },
                                 { "monoOn", 1 }, { "monoFreq", 120 }, { "widthOn", 1 }, { "width", 110 }, { "toneThd", 20 } } },
            { "Console - Analog Character", { { "toneThd", 50 }, { "toneVGain", 20 },
                                              { "satOn", 1 }, { "satDrive", 6 }, { "satHarmonics", 0 }, { "satMix", 40 } } },
            { "Lo-Fi - Telephone", { { "hpfOn", 1 }, { "hpfFreq", 400 }, { "lpfOn", 1 }, { "lpfFreq", 3000 }, { "filterPlacement", in },
                                     { "mfFreq", 1200 }, { "mfGain", 6 }, { "mfQ", 1.5f },
                                     { "satOn", 1 }, { "satDrive", 18 }, { "satVcaClip", 1 }, { "satHarmonics", 70 },
                                     { "widthOn", 1 }, { "width", 0 } } },
        };
        return presets;
    }

    constexpr auto nameProperty = "presetName";
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts, juce::File folder)
    : parameters (apvts), userFolder (std::move (folder))
{
}

juce::File PresetManager::defaultUserFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Paragon PR7-C").getChildFile ("Presets");
}

bool PresetManager::isKept (const juce::String& id)
{
    return id == ParamIDs::outputLevel || id == ParamIDs::toneChannel || id == ParamIDs::scListen || id == ParamIDs::mute;
}

int PresetManager::getNumFactoryPresets() { return (int) factoryPresets().size(); }

juce::String PresetManager::getFactoryName (int index)
{
    return juce::isPositiveAndBelow (index, getNumFactoryPresets()) ? juce::String (factoryPresets()[(size_t) index].name) : juce::String();
}

std::vector<std::pair<juce::String, float>> PresetManager::getFactoryValues (int index)
{
    return juce::isPositiveAndBelow (index, getNumFactoryPresets()) ? factoryPresets()[(size_t) index].values
                                                                    : std::vector<std::pair<juce::String, float>>();
}

std::vector<PresetManager::Preset> PresetManager::getPresets() const
{
    std::vector<Preset> presets;
    for (int i = 0; i < getNumFactoryPresets(); ++i)
        presets.push_back ({ getFactoryName (i), true, i, {} });

    auto files = userFolder.findChildFiles (juce::File::findFiles, false, juce::String ("*") + fileExtension);
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0;
    });
    for (const auto& file : files)
        presets.push_back ({ file.getFileNameWithoutExtension(), false, -1, file });
    return presets;
}

PresetManager::Values PresetManager::valuesOf (const Preset& preset) const
{
    // Every setting a preset sets: its default, unless the preset says otherwise.
    Values values;
    for (auto* parameter : parameters.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter); ranged != nullptr && ! isKept (ranged->getParameterID()))
            values[ranged->getParameterID()] = ranged->convertFrom0to1 (ranged->getDefaultValue());

    if (preset.factory)
    {
        if (juce::isPositiveAndBelow (preset.factoryIndex, getNumFactoryPresets()))
            for (const auto& [id, value] : factoryPresets()[(size_t) preset.factoryIndex].values)
                if (values.count (id) != 0)
                    values[id] = value;
    }
    else if (const auto xml = juce::XmlDocument::parse (preset.file))
    {
        for (const auto* child : xml->getChildWithTagNameIterator ("PARAM"))
        {
            const auto id = child->getStringAttribute ("id");
            if (values.count (id) != 0)   // unknown or kept settings are ignored
                values[id] = (float) child->getDoubleAttribute ("value");
        }
    }
    return values;
}

PresetManager::Values PresetManager::currentValues() const
{
    Values values;
    for (auto* parameter : parameters.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter); ranged != nullptr && ! isKept (ranged->getParameterID()))
            values[ranged->getParameterID()] = ranged->convertFrom0to1 (ranged->getValue());
    return values;
}

void PresetManager::apply (const Values& values)
{
    for (const auto& [id, value] : values)
    {
        if (auto* parameter = parameters.getParameter (id))
        {
            const auto normalised = parameter->convertTo0to1 (value);
            if (! juce::approximatelyEqual (parameter->getValue(), normalised))
            {
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost (normalised);
                parameter->endChangeGesture();
            }
        }
    }
}

void PresetManager::setCurrent (const juce::String& name, const Values& values)
{
    {
        const juce::ScopedLock scoped (lock);
        expected.clear();
        for (const auto& [id, value] : values)
            if (auto* parameter = parameters.getParameter (id))
                expected.emplace_back (parameter, parameter->convertTo0to1 (value));
    }
    parameters.state.setProperty (nameProperty, name, nullptr);
}

void PresetManager::load (const Preset& preset)
{
    const auto values = valuesOf (preset);
    apply (values);
    setCurrent (preset.name, values);
}

void PresetManager::loadFactory (int index)
{
    if (juce::isPositiveAndBelow (index, getNumFactoryPresets()))
        load ({ getFactoryName (index), true, index, {} });
}

const PresetManager::Preset* PresetManager::findCurrent (const std::vector<Preset>& presets) const
{
    const auto name = getCurrentName();
    for (const auto& preset : presets)
        if (preset.name == name)
            return &preset;
    return nullptr;
}

void PresetManager::step (int direction)
{
    const auto presets = getPresets();
    if (presets.empty())
        return;
    const auto* current = findCurrent (presets);
    const auto index = current != nullptr ? (int) (current - presets.data()) : (direction > 0 ? -1 : 0);
    const auto count = (int) presets.size();
    load (presets[(size_t) (((index + direction) % count + count) % count)]);
}

bool PresetManager::save (const juce::String& name)
{
    const auto trimmed = name.trim();
    if (trimmed.isEmpty() || ! userFolder.createDirectory())
        return false;

    const auto values = currentValues();
    juce::XmlElement xml ("PR7CPreset");
    xml.setAttribute ("name", trimmed);
    for (const auto& [id, value] : values)
    {
        auto* child = xml.createNewChildElement ("PARAM");
        child->setAttribute ("id", id);
        child->setAttribute ("value", value);
    }
    const auto file = userFolder.getChildFile (juce::File::createLegalFileName (trimmed) + fileExtension);
    if (! xml.writeTo (file))
        return false;
    setCurrent (file.getFileNameWithoutExtension(), values);
    return true;
}

bool PresetManager::remove (const Preset& preset)
{
    if (preset.factory || ! preset.file.deleteFile())
        return false;
    if (preset.name == getCurrentName())
    {
        // The settings stay as they are, now unnamed.
        const juce::ScopedLock scoped (lock);
        expected.clear();
        parameters.state.setProperty (nameProperty, juce::String(), nullptr);
    }
    return true;
}

juce::String PresetManager::getCurrentName() const
{
    return parameters.state.getProperty (nameProperty, defaultName).toString();
}

bool PresetManager::isModified() const
{
    const juce::ScopedLock scoped (lock);
    if (expected.empty())
        return getCurrentName().isNotEmpty();   // a name we can't find (a deleted preset): call it modified
    for (const auto& [parameter, value] : expected)
        if (std::abs (parameter->getValue() - value) > 1.0e-4f)
            return true;
    return false;
}

void PresetManager::stateRestored()
{
    const auto presets = getPresets();
    if (const auto* current = findCurrent (presets))
    {
        setCurrent (current->name, valuesOf (*current));
        return;
    }
    const juce::ScopedLock scoped (lock);
    expected.clear();
}
