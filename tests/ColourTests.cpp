#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Parameters.h"
#include "PluginProcessor.h"

using Catch::Matchers::WithinAbs;

namespace
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;

    void set (ParagonPR7CAudioProcessor& processor, const juce::String& id, float value)
    {
        auto* parameter = processor.getParameters().getParameter (id);
        REQUIRE (parameter != nullptr);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }

    std::unique_ptr<ParagonPR7CAudioProcessor> makeProcessor()
    {
        auto processor = std::make_unique<ParagonPR7CAudioProcessor>();
        processor->prepareToPlay (sampleRate, blockSize);
        return processor;
    }

    juce::AudioBuffer<float> sine (float hz, float levelDb, int numSamples, int numChannels = 2)
    {
        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        const auto level = juce::Decibels::decibelsToGain (levelDb);
        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample (channel, i, level * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / (float) sampleRate));
        return buffer;
    }

    void process (ParagonPR7CAudioProcessor& processor, juce::AudioBuffer<float>& buffer, const std::function<void (int)>& atBlock = {})
    {
        juce::MidiBuffer midi;
        for (int start = 0; start < buffer.getNumSamples(); start += blockSize)
        {
            if (atBlock)
                atBlock (start);
            juce::AudioBuffer<float> block (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start,
                                            juce::jmin (blockSize, buffer.getNumSamples() - start));
            processor.processBlock (block, midi);
        }
    }

    // The level of one frequency (Goertzel), as a fraction of full scale.
    float levelAt (const juce::AudioBuffer<float>& buffer, float hz, int from, int length, int channel = 0)
    {
        const auto w = juce::MathConstants<double>::twoPi * hz / sampleRate;
        double s1 = 0.0, s2 = 0.0;
        for (int i = from; i < from + length; ++i)
        {
            const auto s0 = buffer.getSample (channel, i) + 2.0 * std::cos (w) * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        return (float) (2.0 * std::sqrt (juce::jmax (0.0, s1 * s1 + s2 * s2 - 2.0 * std::cos (w) * s1 * s2)) / length);
    }

    // A tone's 2nd and 3rd harmonics, relative to it (dB), and its level change.
    struct Harmonics { float second, third, gainDb; };
    Harmonics harmonicsOf (ParagonPR7CAudioProcessor& processor, float levelDb, float hz = 500.0f)
    {
        auto buffer = sine (hz, levelDb, 48000);
        process (processor, buffer);
        const auto fundamental = levelAt (buffer, hz, 24000, 24000);
        return { juce::Decibels::gainToDecibels (levelAt (buffer, hz * 2.0f, 24000, 24000) / fundamental, -200.0f),
                 juce::Decibels::gainToDecibels (levelAt (buffer, hz * 3.0f, 24000, 24000) / fundamental, -200.0f),
                 juce::Decibels::gainToDecibels (fundamental / juce::Decibels::decibelsToGain (levelDb)) };
    }

    float sharpestCorner (const juce::AudioBuffer<float>& buffer, int from, int to, int channel = 0)
    {
        float sharpest = 0.0f;
        const auto* x = buffer.getReadPointer (channel);
        for (int i = juce::jmax (2, from); i < to; ++i)
            sharpest = juce::jmax (sharpest, std::abs (x[i] - 2.0f * x[i - 1] + x[i - 2]));
        return sharpest;
    }

    bool identical (const juce::AudioBuffer<float>& a, int channelA, const juce::AudioBuffer<float>& b, int channelB)
    {
        for (int i = 0; i < a.getNumSamples(); ++i)
            if (! juce::exactlyEqual (a.getSample (channelA, i), b.getSample (channelB, i)))
                return false;
        return true;
    }

    // Music-like test audio: a few tones, different on each side.
    juce::AudioBuffer<float> chord (int numSamples)
    {
        juce::AudioBuffer<float> buffer (2, numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto t = (float) i / (float) sampleRate;
            const auto twoPi = juce::MathConstants<float>::twoPi;
            buffer.setSample (0, i, 0.2f * std::sin (twoPi * 110.0f * t) + 0.1f * std::sin (twoPi * 2500.0f * t) + 0.05f * std::sin (twoPi * 9000.0f * t));
            buffer.setSample (1, i, 0.2f * std::sin (twoPi * 165.0f * t) + 0.1f * std::sin (twoPi * 2500.0f * t + 1.0f) + 0.05f * std::sin (twoPi * 7000.0f * t));
        }
        return buffer;
    }

    // An EQ and filters in use, so the channels' differences show.
    void useEqAndFilters (ParagonPR7CAudioProcessor& processor)
    {
        set (processor, ParamIDs::eqGain ("hmf"), 10.0f);   // a bell at 2.5 kHz
        set (processor, ParamIDs::eqGain ("lf"), -6.0f);
        set (processor, ParamIDs::hpfOn, 1.0f);
        set (processor, ParamIDs::lpfOn, 1.0f);
        set (processor, ParamIDs::lpfFreq, 8000.0f);
    }
}

TEST_CASE ("Colour: the defaults leave the audio untouched, on any channel", "[colour]")
{
    for (const auto channel : { 1, 37, 72 })
    {
        DYNAMIC_SECTION ("channel " << channel)
        {
            auto processor = makeProcessor();
            set (*processor, ParamIDs::toneChannel, (float) channel);
            const auto input = chord (24000);
            auto output = input;
            process (*processor, output);
            CHECK (identical (input, 0, output, 0));
            CHECK (identical (input, 1, output, 1));
        }
    }
}

TEST_CASE ("Colour: THD adds a 2nd harmonic that grows with the level", "[colour]")
{
    auto processor = makeProcessor();
    set (*processor, ParamIDs::toneThd, 100.0f);
    const auto hot = harmonicsOf (*processor, -1.0f);
    CHECK_THAT (hot.second, WithinAbs (-31.5, 2.0));   // about 3% at full scale
    CHECK_THAT (hot.third, WithinAbs (-42.0, 2.0));    // and a smaller 3rd
    processor = makeProcessor();
    set (*processor, ParamIDs::toneThd, 100.0f);
    const auto nominal = harmonicsOf (*processor, -21.0f);   // the console's nominal level
    CHECK_THAT (nominal.second, WithinAbs (-51.5, 2.0));     // 20 dB less: 0.3%
    CHECK (nominal.third < -75.0f);
}

TEST_CASE ("Colour: THD at 50 is about half as much, and keeps the level", "[colour]")
{
    auto processor = makeProcessor();
    set (*processor, ParamIDs::toneThd, 50.0f);
    const auto h = harmonicsOf (*processor, -1.0f);
    CHECK_THAT (h.second, WithinAbs (-37.5, 2.0));
    CHECK_THAT (h.gainDb, WithinAbs (0.0, 0.3));
}

TEST_CASE ("Colour: THD doesn't alias or leave a DC offset", "[colour]")
{
    auto processor = makeProcessor();
    set (*processor, ParamIDs::toneThd, 100.0f);
    // 15 kHz: its 2nd harmonic (30 kHz) would fold back to 18 kHz, its 3rd (45 kHz) to 3 kHz.
    auto buffer = sine (15000.0f, -1.0f, 48000);
    process (*processor, buffer);
    const auto fundamental = levelAt (buffer, 15000.0f, 24000, 24000);
    CHECK (juce::Decibels::gainToDecibels (levelAt (buffer, 18000.0f, 24000, 24000) / fundamental, -200.0f) < -70.0f);
    CHECK (juce::Decibels::gainToDecibels (levelAt (buffer, 3000.0f, 24000, 24000) / fundamental, -200.0f) < -70.0f);
    double sum = 0.0;
    for (int i = 24000; i < 48000; ++i)
        sum += buffer.getSample (0, i);
    CHECK (std::abs (sum / 24000.0) < 1.0e-4);
}

TEST_CASE ("Colour: V GAIN adds the console's hiss and hum", "[colour]")
{
    const auto noiseOf = [] (float vGain)
    {
        auto processor = makeProcessor();
        set (*processor, ParamIDs::toneVGain, vGain);
        juce::AudioBuffer<float> silence (2, 96000);
        silence.clear();
        process (*processor, silence);
        return silence;
    };
    const auto full = noiseOf (100.0f), half = noiseOf (50.0f);
    const auto fullDb = juce::Decibels::gainToDecibels (full.getRMSLevel (0, 48000, 48000));
    CHECK_THAT (fullDb, WithinAbs (-63.4, 1.0));   // hiss at -64, hum at -72
    CHECK_THAT (juce::Decibels::gainToDecibels (half.getRMSLevel (0, 48000, 48000)) - fullDb, WithinAbs (-12.0, 1.0));

    // The hum: a 50 Hz line, well above the hiss around it.
    CHECK (levelAt (full, 50.0f, 48000, 48000) > 10.0f * levelAt (full, 73.0f, 48000, 48000));

    // Each side's hiss is its own: two separate channels, not one copied.
    double both = 0.0, left = 0.0, right = 0.0;
    for (int i = 48000; i < 96000; ++i)
    {
        both += full.getSample (0, i) * full.getSample (1, i);
        left += full.getSample (0, i) * full.getSample (0, i);
        right += full.getSample (1, i) * full.getSample (1, i);
    }
    CHECK (std::abs (both / std::sqrt (left * right)) < 0.3);   // only the hum is shared
}

TEST_CASE ("Colour: channel 1 is the exact design", "[colour]")
{
    auto design = makeProcessor(), channelOne = makeProcessor();
    set (*design, ParamIDs::toneOn, 0.0f);   // off: the design itself
    for (auto* p : { design.get(), channelOne.get() })
        useEqAndFilters (*p);
    auto a = chord (24000), b = chord (24000);
    process (*design, a);
    process (*channelOne, b);
    CHECK (identical (a, 0, b, 0));       // the left side, channel 1, is exact
    CHECK (! identical (a, 1, b, 1));     // the right side, channel 2, isn't quite
}

TEST_CASE ("Colour: each channel is slightly different, and the same every time", "[colour]")
{
    const auto run = [] (int channel, bool on = true)
    {
        auto processor = makeProcessor();
        useEqAndFilters (*processor);
        set (*processor, ParamIDs::toneChannel, (float) channel);
        set (*processor, ParamIDs::toneOn, on ? 1.0f : 0.0f);
        auto buffer = chord (48000);
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());   // the same on both sides, to compare them
        process (*processor, buffer);
        return buffer;
    };
    const auto first = run (30), again = run (30), next = run (31);
    CHECK (identical (first, 0, again, 0));
    CHECK (identical (first, 1, again, 1));
    CHECK (identical (first, 1, next, 0));   // the right side of 30 is channel 31

    // Slightly: the 2.5 kHz boost differs between channels by a fraction of a dB.
    const auto boostDb = [] (const juce::AudioBuffer<float>& b, int side) { return juce::Decibels::gainToDecibels (levelAt (b, 2500.0f, 24000, 24000, side)); };
    const auto difference = std::abs (boostDb (first, 0) - boostDb (next, 0));
    CHECK (difference > 0.001f);
    CHECK (difference < 1.0f);

    // Off: both sides are the design, so the same EQ on both.
    const auto off = run (30, false);
    const auto design = run (1, false);
    CHECK (identical (off, 0, design, 0));
}

TEST_CASE ("Colour: the 72 channels' tolerances stay small", "[colour]")
{
    for (int number = 1; number <= effects::ConsoleChannel::count; ++number)
    {
        const auto c = effects::ConsoleChannel::number (number);
        for (size_t band = 0; band < 5; ++band)
        {
            CHECK (std::abs (c.eqFrequency[band] - 1.0f) <= 0.04f);
            CHECK (std::abs (c.eqGain[band] - 1.0f) <= 0.04f);
        }
        CHECK (std::abs (c.highPass - 1.0f) <= 0.03f);
        CHECK (std::abs (c.second - 1.0f) <= 0.25f);
        CHECK (std::abs (c.hissDb) <= 1.5f);
        CHECK (std::abs (c.humDb) <= 3.0f);
    }
    CHECK (effects::ConsoleChannel::nextTo (72) == 1);
    CHECK (effects::ConsoleChannel::nextTo (1) == 2);
}

TEST_CASE ("Colour: turning THD up or changing channel doesn't click", "[colour]")
{
    const auto check = [] (const std::function<void (ParagonPR7CAudioProcessor&)>& change)
    {
        auto processor = makeProcessor();
        set (*processor, ParamIDs::eqGain ("hmf"), 12.0f);
        set (*processor, ParamIDs::eqGain ("lf"), 12.0f);
        auto buffer = sine (200.0f, -6.0f, 48000);
        process (*processor, buffer, [&] (int start) { if (start == 24064) change (*processor); });
        for (int side = 0; side < 2; ++side)
        {
            const auto steady = juce::jmax (sharpestCorner (buffer, 12000, 24000, side), sharpestCorner (buffer, 40000, 48000, side));
            CHECK (sharpestCorner (buffer, 24000, 40000, side) < steady * 3.0f);
        }
    };
    SECTION ("THD") { check ([] (auto& p) { set (p, ParamIDs::toneThd, 100.0f); }); }
    SECTION ("channel") { check ([] (auto& p) { set (p, ParamIDs::toneChannel, 50.0f); }); }
    SECTION ("colour off") { check ([] (auto& p) { set (p, ParamIDs::toneChannel, 50.0f); set (p, ParamIDs::toneOn, 0.0f); }); }
}

TEST_CASE ("Colour: works on a mono track", "[colour]")
{
    auto processor = makeProcessor();
    juce::AudioProcessor::BusesLayout mono;
    mono.inputBuses.add (juce::AudioChannelSet::mono());
    mono.outputBuses.add (juce::AudioChannelSet::mono());
    REQUIRE (processor->setBusesLayout (mono));
    processor->prepareToPlay (sampleRate, blockSize);
    set (*processor, ParamIDs::toneThd, 100.0f);
    set (*processor, ParamIDs::toneVGain, 100.0f);
    auto buffer = sine (500.0f, -1.0f, 48000, 1);
    process (*processor, buffer);
    const auto fundamental = levelAt (buffer, 500.0f, 24000, 24000);
    CHECK (fundamental > 0.8f);
    CHECK (juce::Decibels::gainToDecibels (levelAt (buffer, 1000.0f, 24000, 24000) / fundamental) > -35.0f);
}
