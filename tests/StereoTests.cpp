#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ChannelRegistry.h"
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

    // A wide test signal: a tone in the middle (mid) and another only in the
    // difference between the sides (side), at each frequency given.
    juce::AudioBuffer<float> wide (float midHz, float sideHz, int numSamples)
    {
        juce::AudioBuffer<float> buffer (2, numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto t = (float) i / (float) sampleRate;
            const auto mid = 0.25f * std::sin (juce::MathConstants<float>::twoPi * midHz * t);
            const auto side = 0.25f * std::sin (juce::MathConstants<float>::twoPi * sideHz * t);
            buffer.setSample (0, i, mid + side);
            buffer.setSample (1, i, mid - side);
        }
        return buffer;
    }

    // The level (dB) of one frequency in the mid or side signal (Goertzel).
    float levelDb (const juce::AudioBuffer<float>& buffer, bool side, float hz, int from, int length)
    {
        const auto w = juce::MathConstants<double>::twoPi * hz / sampleRate;
        double s1 = 0.0, s2 = 0.0;
        for (int i = from; i < from + length; ++i)
        {
            const auto x = 0.5 * (buffer.getSample (0, i) + (side ? -1.0 : 1.0) * buffer.getSample (1, i));
            const auto s0 = x + 2.0 * std::cos (w) * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        return juce::Decibels::gainToDecibels ((float) (2.0 * std::sqrt (juce::jmax (0.0, s1 * s1 + s2 * s2 - 2.0 * std::cos (w) * s1 * s2)) / length), -200.0f);
    }

    // How a module changes the mid and side at two frequencies (dB).
    struct Image { float midChange, sideChange; };
    Image imageChange (ParagonPR7CAudioProcessor& processor, float midHz, float sideHz)
    {
        const auto input = wide (midHz, sideHz, 48000);
        auto output = input;
        process (processor, output);
        return { levelDb (output, false, midHz, 24000, 24000) - levelDb (input, false, midHz, 24000, 24000),
                 levelDb (output, true, sideHz, 24000, 24000) - levelDb (input, true, sideHz, 24000, 24000) };
    }

    float sharpestCorner (const juce::AudioBuffer<float>& buffer, int channel, int from, int to)
    {
        float sharpest = 0.0f;
        const auto* x = buffer.getReadPointer (channel);
        for (int i = juce::jmax (2, from); i < to; ++i)
            sharpest = juce::jmax (sharpest, std::abs (x[i] - 2.0f * x[i - 1] + x[i - 2]));
        return sharpest;
    }
}

TEST_CASE ("Mono maker: the side signal below the frequency goes, everything else stays", "[stereo]")
{
    auto processor = makeProcessor();
    set (*processor, ParamIDs::monoOn, 1.0f);
    set (*processor, ParamIDs::monoFreq, 100.0f);

    // The lows: the side is cut steeply (24 dB an octave), the mid keeps its level.
    const auto low = imageChange (*processor, 40.0f, 40.0f);
    CHECK (low.sideChange < -30.0f);
    CHECK_THAT (low.midChange, WithinAbs (0.0, 0.05));
    processor = makeProcessor();
    set (*processor, ParamIDs::monoOn, 1.0f);
    set (*processor, ParamIDs::monoFreq, 100.0f);
    CHECK (imageChange (*processor, 50.0f, 50.0f).sideChange < -20.0f);   // an octave below

    // Above it, the image is as it was.
    processor = makeProcessor();
    set (*processor, ParamIDs::monoOn, 1.0f);
    set (*processor, ParamIDs::monoFreq, 100.0f);
    const auto high = imageChange (*processor, 2000.0f, 1000.0f);
    CHECK_THAT (high.sideChange, WithinAbs (0.0, 0.05));
    CHECK_THAT (high.midChange, WithinAbs (0.0, 0.05));
}

TEST_CASE ("Mono maker: above the frequency the sides stay in step (no smearing)", "[stereo]")
{
    // A sound only on the left, well above the frequency, must stay only on
    // the left: the mid and side are shifted alike, so they still cancel on
    // the right.
    auto processor = makeProcessor();
    set (*processor, ParamIDs::monoOn, 1.0f);
    set (*processor, ParamIDs::monoFreq, 200.0f);
    juce::AudioBuffer<float> buffer (2, 48000);
    for (int i = 0; i < 48000; ++i)
    {
        buffer.setSample (0, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 3000.0f * (float) i / (float) sampleRate));
        buffer.setSample (1, i, 0.0f);
    }
    process (*processor, buffer);
    CHECK (juce::Decibels::gainToDecibels (buffer.getRMSLevel (1, 24000, 24000) / buffer.getRMSLevel (0, 24000, 24000)) < -40.0f);
}

TEST_CASE ("Stereo width: 0 is mono, 200 doubles the side, and the mid never changes", "[stereo]")
{
    for (const auto [width, expectedSideDb] : { std::pair (0.0f, -200.0f), { 50.0f, -6.02f }, { 200.0f, 6.02f } })
    {
        DYNAMIC_SECTION ("width " << width)
        {
            auto processor = makeProcessor();
            set (*processor, ParamIDs::widthOn, 1.0f);
            set (*processor, ParamIDs::width, width);
            const auto change = imageChange (*processor, 500.0f, 700.0f);
            CHECK_THAT (change.midChange, WithinAbs (0.0, 0.01));
            if (width == 0.0f)
                CHECK (change.sideChange < -100.0f);
            else
                CHECK_THAT (change.sideChange, WithinAbs (expectedSideDb, 0.05));
        }
    }
}

TEST_CASE ("Stereo tools: off, and at 100 percent, leave the audio as it was", "[stereo]")
{
    auto processor = makeProcessor();
    set (*processor, ParamIDs::width, 180.0f);   // the knob moved, but off
    set (*processor, ParamIDs::monoFreq, 300.0f);
    const auto input = wide (60.0f, 80.0f, 24000);
    auto output = input;
    process (*processor, output);
    for (int channel = 0; channel < 2; ++channel)
        for (int i = 0; i < input.getNumSamples(); ++i)
            REQUIRE (juce::exactlyEqual (input.getSample (channel, i), output.getSample (channel, i)));

    processor = makeProcessor();
    set (*processor, ParamIDs::widthOn, 1.0f);
    output = input;
    process (*processor, output);
    for (int channel = 0; channel < 2; ++channel)
        for (int i = 0; i < input.getNumSamples(); ++i)
            REQUIRE_THAT (output.getSample (channel, i), WithinAbs (input.getSample (channel, i), 1.0e-6));
}

TEST_CASE ("Stereo tools: switching on and turning the knobs doesn't click", "[stereo]")
{
    const auto check = [] (const std::function<void (ParagonPR7CAudioProcessor&)>& before, const std::function<void (ParagonPR7CAudioProcessor&)>& change)
    {
        auto processor = makeProcessor();
        before (*processor);
        auto buffer = wide (200.0f, 70.0f, 48000);
        process (*processor, buffer, [&] (int start) { if (start == 24064) change (*processor); });
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto steady = juce::jmax (sharpestCorner (buffer, channel, 12000, 24000), sharpestCorner (buffer, channel, 40000, 48000));
            CHECK (sharpestCorner (buffer, channel, 24000, 40000) < steady * 3.0f);
        }
    };
    SECTION ("mono maker on") { check ([] (auto&) {}, [] (auto& p) { set (p, ParamIDs::monoOn, 1.0f); }); }
    SECTION ("mono maker frequency") { check ([] (auto& p) { set (p, ParamIDs::monoOn, 1.0f); }, [] (auto& p) { set (p, ParamIDs::monoFreq, 400.0f); }); }
    SECTION ("width on") { check ([] (auto& p) { set (p, ParamIDs::width, 200.0f); }, [] (auto& p) { set (p, ParamIDs::widthOn, 1.0f); }); }
    SECTION ("width") { check ([] (auto& p) { set (p, ParamIDs::widthOn, 1.0f); }, [] (auto& p) { set (p, ParamIDs::width, 0.0f); }); }
}

TEST_CASE ("Stereo tools: a mono track is left alone", "[stereo]")
{
    auto processor = makeProcessor();
    juce::AudioProcessor::BusesLayout mono;
    mono.inputBuses.add (juce::AudioChannelSet::mono());
    mono.outputBuses.add (juce::AudioChannelSet::mono());
    REQUIRE (processor->setBusesLayout (mono));
    processor->prepareToPlay (sampleRate, blockSize);
    set (*processor, ParamIDs::monoOn, 1.0f);
    set (*processor, ParamIDs::widthOn, 1.0f);
    set (*processor, ParamIDs::width, 0.0f);
    juce::AudioBuffer<float> buffer (1, 4800);
    for (int i = 0; i < 4800; ++i)
        buffer.setSample (0, i, 0.5f * std::sin ((float) i * 0.01f));
    const auto input = buffer;
    process (*processor, buffer);
    for (int i = 0; i < 4800; ++i)
        REQUIRE (juce::exactlyEqual (input.getSample (0, i), buffer.getSample (0, i)));
}

TEST_CASE ("Stereo mode: DIGITAL puts both sides on the left side's channel", "[stereo]")
{
    const auto run = [] (bool digital)
    {
        auto processor = makeProcessor();
        set (*processor, ParamIDs::eqGain ("hmf"), 10.0f);
        set (*processor, ParamIDs::toneChannel, 20.0f);
        set (*processor, ParamIDs::stereoMode, digital ? 1.0f : 0.0f);
        auto buffer = wide (2500.0f, 2500.0f, 24000);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample (1, i, buffer.getSample (0, i));   // the same on both sides
        process (*processor, buffer);
        bool same = true;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            same = same && juce::exactlyEqual (buffer.getSample (0, i), buffer.getSample (1, i));
        return same;
    };
    CHECK (run (true));
    CHECK (! run (false));   // ANALOG: channels 20 and 21 differ slightly
}

TEST_CASE ("Random channels: ONE and ALL give each PR7-C channels no other uses", "[stereo]")
{
    std::vector<std::unique_ptr<ParagonPR7CAudioProcessor>> strips;
    for (int i = 0; i < 6; ++i)
        strips.push_back (std::make_unique<ParagonPR7CAudioProcessor>());
    set (*strips[5], ParamIDs::stereoMode, 1.0f);   // one in DIGITAL: it takes only one channel

    const auto allDistinct = [&]
    {
        ChannelRegistry::Taken taken {};
        for (auto& strip : strips)
        {
            ChannelRegistry::Taken mine {};
            ChannelRegistry::take (mine, strip->getConsoleChannel(), strip->isAnalog());
            for (size_t c = 1; c < taken.size(); ++c)
            {
                if (mine[c] && taken[c])
                    return false;
                taken[c] = taken[c] || mine[c];
            }
        }
        return true;
    };

    CHECK (! allDistinct());   // they all start on channels 1 / 2
    for (int round = 0; round < 20; ++round)
    {
        ChannelRegistry::rerollAll();
        REQUIRE (allDistinct());
    }

    // ONE moves one PR7-C, to a channel the others don't use.
    for (int round = 0; round < 20; ++round)
    {
        const auto before = strips[2]->getConsoleChannel();
        ChannelRegistry::rerollOne (*strips[2]);
        REQUIRE (strips[2]->getConsoleChannel() != before);
        REQUIRE (allDistinct());
    }
}

TEST_CASE ("Random channels: with no channel free, a re-roll still changes it", "[stereo]")
{
    ChannelRegistry::Taken taken {};
    taken.fill (true);
    juce::Random random (1);
    for (int i = 0; i < 50; ++i)
    {
        const auto picked = ChannelRegistry::pickUnused (taken, true, 5, random);
        REQUIRE (picked != 5);
        REQUIRE (picked >= 1);
        REQUIRE (picked <= 72);
    }
}
