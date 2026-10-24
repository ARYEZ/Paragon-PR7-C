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

    std::unique_ptr<ParagonPR7CAudioProcessor> makeGate (float thresholdDb, float rangeDb = 40.0f)
    {
        auto processor = std::make_unique<ParagonPR7CAudioProcessor>();
        processor->prepareToPlay (sampleRate, blockSize);
        set (*processor, ParamIDs::gateOn, 1.0f);
        set (*processor, ParamIDs::gateThreshold, thresholdDb);
        set (*processor, ParamIDs::gateRange, rangeDb);
        return processor;
    }

    juce::AudioBuffer<float> sine (float hz, float levelDb, int numSamples, std::optional<float> rightLevelDb = {})
    {
        juce::AudioBuffer<float> buffer (2, numSamples);
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto level = juce::Decibels::decibelsToGain (channel == 1 && rightLevelDb ? *rightLevelDb : levelDb);
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample (channel, i, level * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / (float) sampleRate));
        }
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

    float changeDb (const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output, int start, int length, int channel = 0)
    {
        return juce::Decibels::gainToDecibels (output.getRMSLevel (channel, start, length) / input.getRMSLevel (channel, start, length), -200.0f);
    }

    float settledChangeDb (ParagonPR7CAudioProcessor& processor, float hz, float levelDb, int channel = 0, std::optional<float> rightLevelDb = {})
    {
        const auto input = sine (hz, levelDb, 48000, rightLevelDb);
        auto output = input;
        process (processor, output);
        return changeDb (input, output, 24000, 24000, channel);
    }

    float sharpestCorner (const juce::AudioBuffer<float>& buffer, int from, int to)
    {
        float sharpest = 0.0f;
        const auto* x = buffer.getReadPointer (0);
        for (int i = juce::jmax (2, from); i < to; ++i)
            sharpest = juce::jmax (sharpest, std::abs (x[i] - 2.0f * x[i - 1] + x[i - 2]));
        return sharpest;
    }

    // A loud tone for `loudSamples`, then quiet: the gate's gain change over
    // the quiet part, in steps of `step` samples (dB).
    std::vector<float> closingCurve (ParagonPR7CAudioProcessor& gate, int loudSamples, int step, int steps)
    {
        auto input = sine (1000.0f, -10.0f, loudSamples + step * steps);
        for (int channel = 0; channel < 2; ++channel)
            input.applyGain (channel, loudSamples, step * steps, juce::Decibels::decibelsToGain (-50.0f));   // -60 dBFS, under the threshold
        auto output = input;
        process (gate, output);
        std::vector<float> curve;
        for (int i = 0; i < steps; ++i)
            curve.push_back (changeDb (input, output, loudSamples + i * step, step));
        return curve;
    }
}

TEST_CASE ("Gate: below the threshold it turns down by RANGE, above it passes untouched", "[gate]")
{
    auto gate = makeGate (-40.0f, 30.0f);
    CHECK_THAT (settledChangeDb (*gate, 1000.0f, -50.0f), WithinAbs (-30.0, 0.5));
    auto open = makeGate (-40.0f, 30.0f);
    CHECK (std::abs (settledChangeDb (*open, 1000.0f, -30.0f)) < 0.01f);
    CHECK (open->getGainReduction (ParagonPR7CAudioProcessor::Dynamics::gate, 0).takeReduction() < 0.01f);
}

TEST_CASE ("Gate: -30 shifts the threshold 30 dB lower", "[gate]")
{
    auto gate = makeGate (-40.0f);
    set (*gate, ParamIDs::gateMinus30, 1.0f);   // now -70
    CHECK (std::abs (settledChangeDb (*gate, 1000.0f, -50.0f)) < 0.01f);
    CHECK (settledChangeDb (*gate, 1000.0f, -80.0f) < -30.0f);
}

TEST_CASE ("Gate: the expander turns down by its ratio, no further than RANGE", "[gate]")
{
    // 10 dB under the threshold at 2:1: another 10 dB down.
    auto expander = makeGate (-40.0f, 40.0f);
    set (*expander, ParamIDs::gateExpander, 1.0f);
    set (*expander, ParamIDs::gateRatio, 2.0f);
    CHECK_THAT (settledChangeDb (*expander, 1000.0f, -50.0f), WithinAbs (-10.0, 0.6));

    // At 20:1 it would be 190 dB: RANGE stops it at 25.
    auto steep = makeGate (-40.0f, 25.0f);
    set (*steep, ParamIDs::gateExpander, 1.0f);
    set (*steep, ParamIDs::gateRatio, 20.0f);
    CHECK_THAT (settledChangeDb (*steep, 1000.0f, -50.0f), WithinAbs (-25.0, 0.6));
}

TEST_CASE ("Gate: FAST opens quicker than SLOW", "[gate]")
{
    // Quiet, then a loud tone: how open is it 1 ms in?
    const auto openAfter1ms = [] (int attack)
    {
        auto gate = makeGate (-40.0f, 40.0f);
        set (*gate, ParamIDs::gateAttack, (float) attack);
        auto input = sine (1000.0f, -10.0f, 9600);
        for (int channel = 0; channel < 2; ++channel)
            input.applyGain (channel, 0, 4800, juce::Decibels::decibelsToGain (-60.0f));   // -70: closed
        auto output = input;
        process (*gate, output);
        return changeDb (input, output, 4800 + 24, 48);
    };
    CHECK (openAfter1ms (0) > -1.0f);    // FAST: open already
    CHECK (openAfter1ms (2) < -10.0f);   // SLOW: still opening
}

TEST_CASE ("Gate: HOLD keeps it open after the sound drops", "[gate]")
{
    const auto at100ms = [] (float holdMs)
    {
        auto gate = makeGate (-40.0f, 40.0f);
        set (*gate, ParamIDs::gateHold, holdMs);
        set (*gate, ParamIDs::gateRelease, 20.0f);
        return closingCurve (*gate, 9600, 960, 8)[4];   // 80 to 100 ms after it went quiet
    };
    CHECK (at100ms (300.0f) > -0.5f);    // still held open
    CHECK (at100ms (0.0f) < -30.0f);     // long closed
}

TEST_CASE ("Gate: LINEAR closes at a steady rate; the default closes quickly at first", "[gate]")
{
    const auto curve = [] (bool linear)
    {
        auto gate = makeGate (-40.0f, 40.0f);
        set (*gate, ParamIDs::gateHold, 0.0f);
        set (*gate, ParamIDs::gateRelease, 400.0f);
        set (*gate, ParamIDs::gateLinear, linear ? 1.0f : 0.0f);
        return closingCurve (*gate, 9600, 4800, 5);   // 100 ms steps
    };
    const auto exponential = curve (false), linear = curve (true);
    // A quarter of the way through RELEASE (100-200 ms block covers ~150 ms):
    // the exponential curve has fallen much further than the steady one.
    CHECK (exponential[1] < linear[1] - 5.0f);
    // Linear: about 10 dB per 100 ms (40 dB over 400 ms).
    CHECK_THAT (linear[1] - linear[2], WithinAbs (10.0, 2.0));
    // Both are closed by the end.
    CHECK (exponential[4] < -35.0f);
    CHECK (linear[4] < -35.0f);
}

TEST_CASE ("Gate: LINK opens both sides together", "[gate]")
{
    SECTION ("linked: the quiet right side opens with the loud left")
    {
        auto gate = makeGate (-40.0f);
        CHECK (std::abs (settledChangeDb (*gate, 1000.0f, -20.0f, 1, -50.0f)) < 0.01f);
    }
    SECTION ("unlinked: the quiet right side is gated on its own")
    {
        auto gate = makeGate (-40.0f);
        set (*gate, ParamIDs::dynamicsLink, 0.0f);
        CHECK (settledChangeDb (*gate, 1000.0f, -20.0f, 1, -50.0f) < -30.0f);
    }
}

TEST_CASE ("SC FILTER: the gate stops hearing lows below the sidechain HPF, but the audio isn't filtered", "[gate]")
{
    // A 50 Hz hum at -30 dBFS, the gate at -40.
    SECTION ("off: the gate hears it and stays open")
    {
        auto gate = makeGate (-40.0f);
        CHECK (std::abs (settledChangeDb (*gate, 50.0f, -30.0f)) < 0.05f);
    }
    SECTION ("on, HPF at 200 Hz: the gate barely hears it, and closes")
    {
        auto gate = makeGate (-40.0f);
        set (*gate, ParamIDs::scOn, 1.0f);
        set (*gate, ParamIDs::scHpf, 200.0f);
        CHECK (settledChangeDb (*gate, 50.0f, -30.0f) < -30.0f);
    }
}

TEST_CASE ("SC FILTER steers the compressor too", "[gate]")
{
    const auto reduction = [] (bool filtered)
    {
        auto strip = std::make_unique<ParagonPR7CAudioProcessor>();
        strip->prepareToPlay (sampleRate, blockSize);
        set (*strip, ParamIDs::compOn, 1.0f);
        set (*strip, ParamIDs::compThreshold, -30.0f);
        set (*strip, ParamIDs::scOn, filtered ? 1.0f : 0.0f);
        set (*strip, ParamIDs::scHpf, 200.0f);
        return -settledChangeDb (*strip, 50.0f, -10.0f);
    };
    CHECK (reduction (false) > 10.0f);
    CHECK (reduction (true) < 2.0f);
}

TEST_CASE ("LISTEN plays what the gate and compressor hear; HIGH Q makes the filters resonate", "[gate]")
{
    const auto listened = [] (float hz, float hpf, bool highQ)
    {
        auto strip = std::make_unique<ParagonPR7CAudioProcessor>();
        strip->prepareToPlay (sampleRate, blockSize);
        set (*strip, ParamIDs::scOn, 1.0f);
        set (*strip, ParamIDs::scHpf, hpf);
        set (*strip, ParamIDs::scHighQ, highQ ? 1.0f : 0.0f);
        set (*strip, ParamIDs::scListen, 1.0f);
        return settledChangeDb (*strip, hz, -20.0f);
    };
    CHECK (listened (50.0f, 200.0f, false) < -20.0f);   // the hum, filtered out of what we hear
    CHECK (std::abs (listened (2000.0f, 200.0f, false)) < 0.2f);
    const auto atCutoffPlain = listened (100.0f, 100.0f, false), atCutoffResonant = listened (100.0f, 100.0f, true);
    CHECK_THAT (atCutoffPlain, WithinAbs (-3.0, 0.5));
    CHECK (atCutoffResonant > atCutoffPlain + 8.0f);    // HIGH Q: a peak at the cutoff
}

TEST_CASE ("Gate: switching it on, EXP, or LISTEN doesn't click", "[gate]")
{
    const auto check = [] (const std::function<void (ParagonPR7CAudioProcessor&)>& before, const std::function<void (ParagonPR7CAudioProcessor&)>& change)
    {
        auto gate = makeGate (-40.0f, 20.0f);
        before (*gate);
        auto buffer = sine (200.0f, -45.0f, 48000);   // just under the threshold
        process (*gate, buffer, [&] (int start) { if (start == 24064) change (*gate); });
        const auto steady = juce::jmax (sharpestCorner (buffer, 12000, 24000), sharpestCorner (buffer, 40000, 48000));
        CHECK (sharpestCorner (buffer, 24000, 40000) < steady * 3.0f);
    };
    SECTION ("on") { check ([] (auto& g) { set (g, ParamIDs::gateOn, 0.0f); }, [] (auto& g) { set (g, ParamIDs::gateOn, 1.0f); }); }
    // SLOW attack: switching from a closed gate (-20 dB) to the expander
    // (-5 dB) opens it by 15 dB, and with a faster attack that's the attack
    // itself, not the switch.
    SECTION ("gate to expander") { check ([] (auto& g) { set (g, ParamIDs::gateAttack, 2.0f); }, [] (auto& g) { set (g, ParamIDs::gateExpander, 1.0f); }); }
    SECTION ("LISTEN") { check ([] (auto& g) { set (g, ParamIDs::gateOn, 0.0f); }, [] (auto& g) { set (g, ParamIDs::scListen, 1.0f); }); }
}
