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

    std::unique_ptr<ParagonPR7CAudioProcessor> makeSaturation (float driveDb, float harmonicsPercent = 30.0f)
    {
        auto processor = std::make_unique<ParagonPR7CAudioProcessor>();
        processor->prepareToPlay (sampleRate, blockSize);
        set (*processor, ParamIDs::satOn, 1.0f);
        set (*processor, ParamIDs::satDrive, driveDb);
        set (*processor, ParamIDs::satHarmonics, harmonicsPercent);
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
    float levelAt (const juce::AudioBuffer<float>& buffer, float hz, int from, int length)
    {
        const auto w = juce::MathConstants<double>::twoPi * hz / sampleRate;
        double s1 = 0.0, s2 = 0.0;
        for (int i = from; i < from + length; ++i)
        {
            const auto s0 = buffer.getSample (0, i) + 2.0 * std::cos (w) * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        return (float) (2.0 * std::sqrt (juce::jmax (0.0, s1 * s1 + s2 * s2 - 2.0 * std::cos (w) * s1 * s2)) / length);
    }

    // A tone's 2nd and 3rd harmonics, relative to it (dB).
    struct Harmonics { float second, third; };
    Harmonics harmonicsOf (ParagonPR7CAudioProcessor& processor, float levelDb, float hz = 500.0f)
    {
        auto buffer = sine (hz, levelDb, 48000);
        process (processor, buffer);
        const auto fundamental = levelAt (buffer, hz, 24000, 24000);
        return { juce::Decibels::gainToDecibels (levelAt (buffer, hz * 2.0f, 24000, 24000) / fundamental, -200.0f),
                 juce::Decibels::gainToDecibels (levelAt (buffer, hz * 3.0f, 24000, 24000) / fundamental, -200.0f) };
    }

    float sharpestCorner (const juce::AudioBuffer<float>& buffer, int from, int to)
    {
        float sharpest = 0.0f;
        const auto* x = buffer.getReadPointer (0);
        for (int i = juce::jmax (2, from); i < to; ++i)
            sharpest = juce::jmax (sharpest, std::abs (x[i] - 2.0f * x[i - 1] + x[i - 2]));
        return sharpest;
    }
}

TEST_CASE ("Saturation: turning DRIVE up keeps quiet sounds at the same level", "[sat]")
{
    for (const auto drive : { 0.0f, 12.0f, 24.0f })
    {
        DYNAMIC_SECTION ("drive " << drive)
        {
            auto sat = makeSaturation (drive);
            const auto input = sine (1000.0f, -50.0f, 48000);
            auto output = input;
            process (*sat, output);
            CHECK_THAT (juce::Decibels::gainToDecibels (output.getRMSLevel (0, 24000, 24000) / input.getRMSLevel (0, 24000, 24000)), WithinAbs (0.0, 0.3));
        }
    }
}

TEST_CASE ("Saturation: more DRIVE, more harmonics", "[sat]")
{
    // A tone at the console's nominal level (-20 dBFS).
    auto light = makeSaturation (0.0f, 50.0f);
    auto heavy = makeSaturation (24.0f, 50.0f);
    const auto lightHarmonics = harmonicsOf (*light, -20.0f), heavyHarmonics = harmonicsOf (*heavy, -20.0f);
    CHECK (lightHarmonics.third < -50.0f);                          // barely coloured at nominal level with no drive
    CHECK (heavyHarmonics.third > lightHarmonics.third + 20.0f);
    CHECK (heavyHarmonics.second > lightHarmonics.second + 20.0f);
}

TEST_CASE ("Saturation: HARMONICS moves the colour from 2nd (warm) to 3rd (gritty)", "[sat]")
{
    // DRIVE +18: well into the saturation at the console's nominal level.
    auto warm = makeSaturation (18.0f, 0.0f);
    auto gritty = makeSaturation (18.0f, 100.0f);
    const auto w = harmonicsOf (*warm, -20.0f), g = harmonicsOf (*gritty, -20.0f);
    CHECK (w.second > w.third + 6.0f);    // valve-like: the 2nd leads
    CHECK (g.third > g.second + 30.0f);   // transistor-like: the 3rd, and almost no 2nd
    CHECK (w.second > -40.0f);
    CHECK (g.third > -40.0f);
}

TEST_CASE ("Saturation: VCA CLIP adds crunch to loud peaks", "[sat]")
{
    auto plain = makeSaturation (12.0f, 100.0f);
    auto clipped = makeSaturation (12.0f, 100.0f);
    set (*clipped, ParamIDs::satVcaClip, 1.0f);
    CHECK (harmonicsOf (*clipped, -6.0f).third > harmonicsOf (*plain, -6.0f).third + 3.0f);
}

TEST_CASE ("Saturation: MIX at 0 is clean", "[sat]")
{
    auto sat = makeSaturation (24.0f, 50.0f);
    set (*sat, ParamIDs::satMix, 0.0f);
    const auto h = harmonicsOf (*sat, -10.0f);
    CHECK (h.second < -90.0f);
    CHECK (h.third < -90.0f);
}

TEST_CASE ("Saturation: no DC offset from the valve curve", "[sat]")
{
    auto sat = makeSaturation (24.0f, 0.0f);
    auto buffer = sine (100.0f, -10.0f, 96000);
    process (*sat, buffer);
    double sum = 0.0;
    for (int i = 48000; i < 96000; ++i)
        sum += buffer.getSample (0, i);
    CHECK (std::abs (sum / 48000.0) < 1.0e-3);
}

TEST_CASE ("Saturation: harmonics above the audio range don't fold back down (aliasing)", "[sat]")
{
    // A 9 kHz tone, driven hard: its 3rd harmonic is at 27 kHz, beyond what
    // 48 kHz can hold. Without oversampling it would fold back to 21 kHz.
    auto sat = makeSaturation (24.0f, 100.0f);
    auto buffer = sine (9000.0f, -10.0f, 48000);
    process (*sat, buffer);
    const auto fundamental = levelAt (buffer, 9000.0f, 24000, 24000);
    CHECK (juce::Decibels::gainToDecibels (levelAt (buffer, 21000.0f, 24000, 24000) / fundamental, -200.0f) < -50.0f);
    CHECK (juce::Decibels::gainToDecibels (levelAt (buffer, 3000.0f, 24000, 24000) / fundamental, -200.0f) < -60.0f);   // 5th harmonic's fold
}

TEST_CASE ("Saturation: works on a mono track", "[sat]")
{
    auto sat = makeSaturation (12.0f, 100.0f);
    juce::AudioProcessor::BusesLayout mono;
    mono.inputBuses.add (juce::AudioChannelSet::mono());
    mono.outputBuses.add (juce::AudioChannelSet::mono());
    REQUIRE (sat->setBusesLayout (mono));
    sat->prepareToPlay (sampleRate, blockSize);
    auto buffer = sine (500.0f, -20.0f, 48000, 1);
    process (*sat, buffer);
    const auto fundamental = levelAt (buffer, 500.0f, 24000, 24000);
    CHECK (fundamental > 0.05f);
    CHECK (juce::Decibels::gainToDecibels (levelAt (buffer, 1500.0f, 24000, 24000) / fundamental) > -50.0f);
}

TEST_CASE ("Saturation: switching it on, VCA CLIP or HARMONICS doesn't click", "[sat]")
{
    const auto check = [] (const std::function<void (ParagonPR7CAudioProcessor&)>& before, const std::function<void (ParagonPR7CAudioProcessor&)>& change)
    {
        auto sat = makeSaturation (12.0f);
        before (*sat);
        auto buffer = sine (200.0f, -20.0f, 48000);
        process (*sat, buffer, [&] (int start) { if (start == 24064) change (*sat); });
        const auto steady = juce::jmax (sharpestCorner (buffer, 12000, 24000), sharpestCorner (buffer, 40000, 48000));
        CHECK (sharpestCorner (buffer, 24000, 40000) < steady * 3.0f);
    };
    SECTION ("on") { check ([] (auto& s) { set (s, ParamIDs::satOn, 0.0f); }, [] (auto& s) { set (s, ParamIDs::satOn, 1.0f); }); }
    SECTION ("VCA CLIP") { check ([] (auto&) {}, [] (auto& s) { set (s, ParamIDs::satVcaClip, 1.0f); }); }
    SECTION ("HARMONICS") { check ([] (auto&) {}, [] (auto& s) { set (s, ParamIDs::satHarmonics, 100.0f); }); }
}
