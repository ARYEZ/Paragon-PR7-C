#include <catch2/catch_test_macros.hpp>
#include <set>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Parameters.h"
#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include "PresetManager.h"

using Catch::Matchers::WithinAbs;

namespace
{
    float valueOf (ParagonPR7CAudioProcessor& processor, const juce::String& id)
    {
        auto* parameter = processor.getParameters().getParameter (id);
        REQUIRE (parameter != nullptr);
        return parameter->convertFrom0to1 (parameter->getValue());
    }

    void set (ParagonPR7CAudioProcessor& processor, const juce::String& id, float value)
    {
        auto* parameter = processor.getParameters().getParameter (id);
        REQUIRE (parameter != nullptr);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }

    // A preset manager whose user presets go in a fresh temporary folder.
    struct Scratch
    {
        juce::TemporaryFile folder;
        Scratch() { folder.getFile().createDirectory(); }
        ~Scratch() { folder.getFile().deleteRecursively(); }
        juce::File get() const { return folder.getFile(); }
    };

    int indexOf (const std::vector<PresetManager::Preset>& list, const juce::String& name)
    {
        for (size_t i = 0; i < list.size(); ++i)
            if (list[i].name == name)
                return (int) i;
        return -1;
    }
}

TEST_CASE ("Presets: every factory preset names real settings, with values in range", "[presets]")
{
    ParagonPR7CAudioProcessor processor;
    REQUIRE (PresetManager::getNumFactoryPresets() > 10);
    REQUIRE (PresetManager::getFactoryName (0) == PresetManager::defaultName);
    for (int i = 0; i < PresetManager::getNumFactoryPresets(); ++i)
    {
        INFO (PresetManager::getFactoryName (i).toStdString());
        for (const auto& [id, value] : PresetManager::getFactoryValues (i))
        {
            INFO (id.toStdString());
            auto* parameter = processor.getParameters().getParameter (id);
            REQUIRE (parameter != nullptr);
            REQUIRE (! PresetManager::isKept (id));
            const auto range = parameter->getNormalisableRange();
            CHECK (value >= range.start);
            CHECK (value <= range.end);
        }
    }
}

TEST_CASE ("Presets: a new PR7-C is on Default, unmodified", "[presets]")
{
    ParagonPR7CAudioProcessor processor;
    CHECK (processor.getPresets().getCurrentName() == "Default");
    CHECK (! processor.getPresets().isModified());
}

TEST_CASE ("Presets: loading sets everything, and leaves the fader, channel, LISTEN and MUTE alone", "[presets]")
{
    ParagonPR7CAudioProcessor processor;
    Scratch scratch;
    PresetManager presets (processor.getParameters(), scratch.get());
    set (processor, ParamIDs::outputLevel, -12.0f);
    set (processor, ParamIDs::toneChannel, 40.0f);
    set (processor, ParamIDs::mute, 1.0f);
    set (processor, ParamIDs::satOn, 1.0f);   // not in the kick preset: back to its default (off)

    const auto list = presets.getPresets();
    const auto kick = indexOf (list, "Kick Punch");
    REQUIRE (kick >= 0);
    presets.load (list[(size_t) kick]);
    CHECK (presets.getCurrentName() == "Kick Punch");
    CHECK (presets.getCurrentSlot() == "A01");
    CHECK (! presets.isModified());
    CHECK_THAT (valueOf (processor, ParamIDs::gateOn), WithinAbs (1.0, 1e-6));
    CHECK_THAT (valueOf (processor, ParamIDs::eqGain ("lf")), WithinAbs (4.0, 0.01));
    CHECK_THAT (valueOf (processor, ParamIDs::compCharacter), WithinAbs (2.0, 1e-6));   // FET
    CHECK_THAT (valueOf (processor, ParamIDs::satOn), WithinAbs (0.0, 1e-6));
    CHECK_THAT (valueOf (processor, ParamIDs::outputLevel), WithinAbs (-12.0, 0.01));
    CHECK_THAT (valueOf (processor, ParamIDs::toneChannel), WithinAbs (40.0, 1e-6));
    CHECK_THAT (valueOf (processor, ParamIDs::mute), WithinAbs (1.0, 1e-6));

    // Changing a setting marks it modified; the fader doesn't (it isn't part of a preset).
    set (processor, ParamIDs::outputLevel, 3.0f);
    CHECK (! presets.isModified());
    set (processor, ParamIDs::compThreshold, -30.0f);
    CHECK (presets.isModified());
    presets.load (list[(size_t) kick]);
    CHECK (! presets.isModified());
}

TEST_CASE ("Presets: the arrows step through the list and wrap round", "[presets]")
{
    ParagonPR7CAudioProcessor processor;
    Scratch scratch;
    PresetManager presets (processor.getParameters(), scratch.get());
    const auto list = presets.getPresets();
    presets.step (1);
    CHECK (presets.getCurrentName() == list[1].name);
    presets.step (-1);
    presets.step (-1);
    CHECK (presets.getCurrentName() == list.back().name);
    presets.step (1);
    CHECK (presets.getCurrentName() == list.front().name);
}

TEST_CASE ("Presets: SAVE makes a user preset that loads back exactly", "[presets]")
{
    ParagonPR7CAudioProcessor processor;
    Scratch scratch;
    PresetManager presets (processor.getParameters(), scratch.get());
    set (processor, ParamIDs::compOn, 1.0f);
    set (processor, ParamIDs::compThreshold, -27.5f);
    set (processor, ParamIDs::eqFreq ("hmf"), 1234.0f);
    set (processor, ParamIDs::limitMode, 2.0f);
    set (processor, ParamIDs::width, 150.0f);
    REQUIRE (presets.save ("My Vocal"));
    CHECK (presets.getCurrentName() == "My Vocal");
    CHECK (! presets.isModified());
    CHECK (scratch.get().getChildFile ("My Vocal.pr7cpreset").existsAsFile());

    // Somewhere else, then back.
    presets.loadFactory (0);
    CHECK_THAT (valueOf (processor, ParamIDs::compThreshold), WithinAbs (-18.0, 0.01));
    auto list = presets.getPresets();
    const auto mine = indexOf (list, "My Vocal");
    REQUIRE (mine == PresetManager::getNumFactoryPresets());   // after the factory ones
    CHECK (! list[(size_t) mine].factory);
    presets.load (list[(size_t) mine]);
    CHECK_THAT (valueOf (processor, ParamIDs::compThreshold), WithinAbs (-27.5, 0.01));
    CHECK_THAT (valueOf (processor, ParamIDs::eqFreq ("hmf")), WithinAbs (1234.0, 1.0));
    CHECK_THAT (valueOf (processor, ParamIDs::limitMode), WithinAbs (2.0, 1e-6));
    CHECK_THAT (valueOf (processor, ParamIDs::width), WithinAbs (150.0, 0.01));
    CHECK (! presets.isModified());

    // User presets are listed A to Z, and can be deleted.
    REQUIRE (presets.save ("A Bass"));
    list = presets.getPresets();
    CHECK (indexOf (list, "A Bass") < indexOf (list, "My Vocal"));
    CHECK (presets.remove (list[(size_t) indexOf (list, "My Vocal")]));
    CHECK (indexOf (presets.getPresets(), "My Vocal") == -1);
    CHECK (! presets.remove (presets.getPresets()[0]));   // factory presets can't be deleted
}

TEST_CASE ("Presets: a song remembers its preset's name and whether it was changed", "[presets]")
{
    juce::MemoryBlock saved;
    {
        ParagonPR7CAudioProcessor processor;
        processor.getPresets().loadFactory (4);
        set (processor, ParamIDs::compMakeUp, 7.0f);
        processor.getStateInformation (saved);
    }
    ParagonPR7CAudioProcessor restored;
    restored.setStateInformation (saved.getData(), (int) saved.getSize());
    CHECK (restored.getPresets().getCurrentName() == PresetManager::getFactoryName (4));
    CHECK (restored.getPresets().isModified());
    CHECK_THAT (valueOf (restored, ParamIDs::compMakeUp), WithinAbs (7.0, 0.01));
}

TEST_CASE ("Presets: every factory preset plays without blowing up", "[presets]")
{
    for (int i = 0; i < PresetManager::getNumFactoryPresets(); ++i)
    {
        DYNAMIC_SECTION (i)
        {
            ParagonPR7CAudioProcessor processor;
            processor.prepareToPlay (48000.0, 256);
            processor.getPresets().loadFactory (i);
            juce::AudioBuffer<float> buffer (2, 48000);
            for (int s = 0; s < buffer.getNumSamples(); ++s)
            {
                const auto t = (float) s / 48000.0f;
                buffer.setSample (0, s, 0.3f * std::sin (juce::MathConstants<float>::twoPi * 110.0f * t) + 0.1f * std::sin (juce::MathConstants<float>::twoPi * 3000.0f * t));
                buffer.setSample (1, s, 0.3f * std::sin (juce::MathConstants<float>::twoPi * 165.0f * t));
            }
            juce::MidiBuffer midi;
            for (int start = 0; start < buffer.getNumSamples(); start += 256)
            {
                juce::AudioBuffer<float> block (buffer.getArrayOfWritePointers(), 2, start, juce::jmin (256, buffer.getNumSamples() - start));
                processor.processBlock (block, midi);
            }
            for (int channel = 0; channel < 2; ++channel)
            {
                const auto peak = buffer.getMagnitude (channel, 0, buffer.getNumSamples());
                CHECK (std::isfinite (peak));
                CHECK (peak < 2.0f);
                CHECK (buffer.getRMSLevel (channel, 24000, 24000) > 0.01f);   // and something comes out
            }
        }
    }
}

TEST_CASE ("Presets: filed as PR7-X files its rigs (banks A to G, genre and signature sets)", "[presets]")
{
    const auto& presets = factoryPresets();
    REQUIRE (presets.front().name == PresetManager::defaultName);
    CHECK (presets.size() > 150);

    std::set<juce::String> names, slots;
    for (const auto& preset : presets)
    {
        INFO (preset.name.toStdString());
        CHECK (names.insert (preset.name).second);       // every name once
        if (preset.bank < 0)
            continue;
        CHECK (preset.bank < FactoryBanks::names.size());
        CHECK (slots.insert (preset.slot()).second);     // every slot once
        CHECK (preset.tags.size() > 0);
        if (preset.set.isEmpty())
            CHECK ((preset.number >= 1 && preset.number <= 10));
    }

    // Each genre (11 to 19) and signature sound (31 to 38): six parts, at the
    // same number in every bank but synths and keys.
    const auto checkSet = [&] (const juce::String& set, int number)
    {
        INFO (set.toStdString());
        std::set<int> banks;
        for (const auto& preset : presets)
            if (preset.set == set)
            {
                CHECK (preset.number == number);
                banks.insert (preset.bank);
            }
        CHECK (banks == std::set<int> { FactoryBanks::drums, FactoryBanks::bass, FactoryBanks::guitars, FactoryBanks::vocals,
                                         FactoryBanks::mix, FactoryBanks::master });
    };
    for (int i = 0; i < FactoryBanks::genres.size(); ++i)
        checkSet ("Genres/" + FactoryBanks::genres[i], 11 + i);
    for (int i = 0; i < FactoryBanks::signatureSounds.size(); ++i)
        checkSet ("Signature Sounds/" + FactoryBanks::signatureSounds[i], 31 + i);

    // In bank order, so the arrows step A01, A02... G38.
    for (size_t i = 2; i < presets.size(); ++i)
        CHECK ((presets[i - 1].bank < presets[i].bank || (presets[i - 1].bank == presets[i].bank && presets[i - 1].number < presets[i].number)));
}
