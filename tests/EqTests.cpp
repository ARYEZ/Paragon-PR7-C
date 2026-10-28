#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Parameters.h"
#include "Latency.h"
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

    juce::AudioBuffer<float> sine (float hz, float level, int numSamples, int numChannels = 2)
    {
        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        for (int channel = 0; channel < numChannels; ++channel)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample (channel, i, level * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / (float) sampleRate));
        return buffer;
    }

    // Processes in blocks, calling `atBlock (startSample)` before each one
    // (to change settings part-way through).
    void process (ParagonPR7CAudioProcessor& processor, juce::AudioBuffer<float>& buffer,
                  const std::function<void (int)>& atBlock = {})
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

    // What the strip does to a sine at `hz`, in dB, once settled.
    float responseDb (ParagonPR7CAudioProcessor& processor, float hz, int channel = 0)
    {
        processor.reset();
        const auto numSamples = 24000;
        const auto input = sine (hz, 0.1f, numSamples);
        auto output = input;
        process (processor, output);
        return juce::Decibels::gainToDecibels (output.getRMSLevel (channel, numSamples / 2, numSamples / 2)
                                               / input.getRMSLevel (channel, numSamples / 2, numSamples / 2));
    }

    std::unique_ptr<ParagonPR7CAudioProcessor> makeStrip()
    {
        auto processor = std::make_unique<ParagonPR7CAudioProcessor>();
        processor->prepareToPlay (sampleRate, blockSize);
        return processor;
    }

    // The sharpest corner in a signal: the biggest second difference. A
    // click (a step or a kink) shows as a spike far above a smooth wave's.
    float sharpestCorner (const juce::AudioBuffer<float>& buffer, int from, int to)
    {
        float sharpest = 0.0f;
        const auto* samples = buffer.getReadPointer (0);
        for (int i = juce::jmax (2, from); i < to; ++i)
            sharpest = juce::jmax (sharpest, std::abs (samples[i] - 2.0f * samples[i - 1] + samples[i - 2]));
        return sharpest;
    }

    // Switches something half-way through a tone, and checks the switch is
    // no sharper than the steady tone either side of it (within 3 times: a
    // smooth glide stays well under that, while a real click measures
    // hundreds of times sharper).
    void checkSwitchIsSmooth (ParagonPR7CAudioProcessor& processor, float hz, const std::function<void()>& change)
    {
        const auto numSamples = 48000;
        auto output = sine (hz, 0.1f, numSamples);
        process (processor, output, [&] (int start) { if (start == 24064) change(); });
        const auto steady = juce::jmax (sharpestCorner (output, 12000, 24000), sharpestCorner (output, 36000, numSamples));
        CHECK (sharpestCorner (output, 24000, 36000) < steady * 3.0f);
    }
}

TEST_CASE ("Each EQ bell boosts at its frequency and leaves the rest alone", "[eq]")
{
    for (const auto* band : { "hmf", "mf", "lmf" })
    {
        DYNAMIC_SECTION (band)
        {
            auto strip = makeStrip();
            const auto hz = strip->getParameters().getRawParameterValue (ParamIDs::eqFreq (band))->load();
            set (*strip, ParamIDs::eqGain (band), 12.0f);
            CHECK_THAT (responseDb (*strip, hz), WithinAbs (12.0, 0.3));
            // Three octaves either side (kept below the top of the audio range).
            CHECK (std::abs (responseDb (*strip, juce::jmin (hz * 8.0f, 16000.0f))) < 1.0f);
            CHECK (std::abs (responseDb (*strip, hz / 8.0f)) < 1.0f);
        }
    }
}

TEST_CASE ("HF and LF shelves lift everything beyond their frequency", "[eq]")
{
    SECTION ("HF, a high shelf at 5 kHz")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::eqFreq ("hf"), 5000.0f);
        set (*strip, ParamIDs::eqGain ("hf"), 10.0f);
        CHECK_THAT (responseDb (*strip, 5000.0f), WithinAbs (5.0, 0.5));   // half way at the frequency
        CHECK_THAT (responseDb (*strip, 18000.0f), WithinAbs (10.0, 1.0));
        CHECK (std::abs (responseDb (*strip, 300.0f)) < 0.3f);
    }
    SECTION ("LF, a low shelf at 100 Hz")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::eqFreq ("lf"), 100.0f);
        set (*strip, ParamIDs::eqGain ("lf"), -10.0f);
        CHECK_THAT (responseDb (*strip, 100.0f), WithinAbs (-5.0, 0.5));
        CHECK_THAT (responseDb (*strip, 25.0f), WithinAbs (-10.0, 1.0));
        CHECK (std::abs (responseDb (*strip, 2000.0f)) < 0.3f);
    }
}

TEST_CASE ("SHELF off turns HF into a bell", "[eq]")
{
    auto strip = makeStrip();
    set (*strip, ParamIDs::eqFreq ("hf"), 5000.0f);
    set (*strip, ParamIDs::eqGain ("hf"), 10.0f);
    set (*strip, ParamIDs::eqShelf ("hf"), 0.0f);
    CHECK_THAT (responseDb (*strip, 5000.0f), WithinAbs (10.0, 0.3));
    CHECK (responseDb (*strip, 18000.0f) < 4.0f);   // a shelf would still be near +10 here
}

TEST_CASE ("Q sets how wide a bell is", "[eq]")
{
    const auto octaveAway = [] (float q)
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::eqFreq ("hmf"), 2000.0f);
        set (*strip, ParamIDs::eqGain ("hmf"), 12.0f);
        set (*strip, ParamIDs::eqQ ("hmf"), q);
        return responseDb (*strip, 1000.0f);
    };
    const auto wide = octaveAway (0.5f), narrow = octaveAway (4.0f);
    CHECK (wide > 5.0f);     // most of a 12 dB boost still there an octave away
    CHECK (narrow < 1.5f);   // almost none
}

TEST_CASE ("Bells narrow as they boost harder (proportional Q)", "[eq]")
{
    // How much of the boost is left an octave away, at a small and a big boost.
    const auto shareAnOctaveAway = [] (float gain)
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::eqFreq ("mf"), 600.0f);
        set (*strip, ParamIDs::eqGain ("mf"), gain);
        return responseDb (*strip, 1200.0f) / gain;
    };
    CHECK (shareAnOctaveAway (15.0f) < shareAnOctaveAway (3.0f) - 0.05f);
    // And the Q knob reads true at ±6 dB.
    CHECK_THAT (effects::EqBand::proportionalQ (2.0f, 6.0f), WithinAbs (2.0, 1.0e-6));
}

TEST_CASE ("A band switched off leaves the audio untouched", "[eq]")
{
    auto strip = makeStrip();
    set (*strip, ParamIDs::eqGain ("hmf"), 12.0f);
    set (*strip, ParamIDs::eqOn ("hmf"), 0.0f);
    const auto input = sine (2500.0f, 0.1f, 9600);
    auto output = input;
    process (*strip, output);
    for (int i = 0; i < input.getNumSamples(); ++i)
        REQUIRE_THAT (output.getSample (0, i), WithinAbs (delayedInput (input, 0, i), 1.0e-6));
}

TEST_CASE ("HPF and LPF roll off at 12 dB an octave", "[eq]")
{
    SECTION ("HPF at 100 Hz")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::hpfOn, 1.0f);
        set (*strip, ParamIDs::hpfFreq, 100.0f);
        CHECK_THAT (responseDb (*strip, 100.0f), WithinAbs (-3.0, 0.3));
        CHECK_THAT (responseDb (*strip, 50.0f), WithinAbs (-12.3, 0.6));
        CHECK (responseDb (*strip, 1000.0f) > -0.1f);
    }
    SECTION ("LPF at 2 kHz")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::lpfOn, 1.0f);
        set (*strip, ParamIDs::lpfFreq, 2000.0f);
        CHECK_THAT (responseDb (*strip, 2000.0f), WithinAbs (-3.0, 0.3));
        CHECK_THAT (responseDb (*strip, 4000.0f), WithinAbs (-12.3, 0.8));
        CHECK (responseDb (*strip, 200.0f) > -0.1f);
    }
}

TEST_CASE ("IN and POST filter the audio; SC leaves it alone", "[eq]")
{
    for (const auto placement : { 0, 1, 2 })
    {
        DYNAMIC_SECTION (ParamRanges::filterPlacements[placement])
        {
            auto strip = makeStrip();
            set (*strip, ParamIDs::hpfOn, 1.0f);
            set (*strip, ParamIDs::hpfFreq, 1000.0f);
            set (*strip, ParamIDs::filterPlacement, (float) placement);
            const auto at250 = responseDb (*strip, 250.0f);
            if (placement == 2)
                CHECK (std::abs (at250) < 0.01f);
            else
                CHECK (at250 < -20.0f);
        }
    }
}

TEST_CASE ("The EQ and filters treat both channels alike, and work on a mono track", "[eq]")
{
    auto strip = makeStrip();
    set (*strip, ParamIDs::eqGain ("mf"), 9.0f);
    set (*strip, ParamIDs::hpfOn, 1.0f);

    // By default the right side is console channel 2, a hair different from
    // channel 1 on the left. With the console colour off, both are the design.
    const auto difference = std::abs (responseDb (*strip, 600.0f, 1) - responseDb (*strip, 600.0f, 0));
    CHECK (difference > 0.0001f);
    CHECK (difference < 0.5f);
    set (*strip, ParamIDs::toneOn, 0.0f);
    CHECK_THAT (responseDb (*strip, 600.0f, 1), WithinAbs (responseDb (*strip, 600.0f, 0), 0.01));
    set (*strip, ParamIDs::toneOn, 1.0f);

    // Mono: one channel in and out.
    juce::AudioProcessor::BusesLayout mono;
    mono.inputBuses.add (juce::AudioChannelSet::mono());
    mono.outputBuses.add (juce::AudioChannelSet::mono());
    REQUIRE (strip->setBusesLayout (mono));
    strip->prepareToPlay (sampleRate, blockSize);
    const auto input = sine (600.0f, 0.1f, 24000, 1);
    auto output = input;
    process (*strip, output);
    CHECK_THAT (juce::Decibels::gainToDecibels (output.getRMSLevel (0, 12000, 12000) / input.getRMSLevel (0, 12000, 12000)), WithinAbs (9.0, 0.3));
}

TEST_CASE ("Switching the EQ and filters mid-signal doesn't click", "[eq]")
{
    SECTION ("a band on, with a big boost")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::eqGain ("hmf"), 12.0f);
        set (*strip, ParamIDs::eqOn ("hmf"), 0.0f);
        checkSwitchIsSmooth (*strip, 2500.0f, [&] { set (*strip, ParamIDs::eqOn ("hmf"), 1.0f); });
    }
    SECTION ("a low band off, with a big boost (its ring dies away rather than being cut)")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::eqShelf ("lf"), 0.0f);
        set (*strip, ParamIDs::eqFreq ("lf"), 30.0f);
        set (*strip, ParamIDs::eqGain ("lf"), 12.0f);
        checkSwitchIsSmooth (*strip, 40.0f, [&] { set (*strip, ParamIDs::eqOn ("lf"), 0.0f); });
    }
    SECTION ("shelf to bell")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::eqFreq ("hf"), 5000.0f);
        set (*strip, ParamIDs::eqGain ("hf"), 10.0f);
        checkSwitchIsSmooth (*strip, 3000.0f, [&] { set (*strip, ParamIDs::eqShelf ("hf"), 0.0f); });
    }
    SECTION ("a jump in frequency")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::eqFreq ("hmf"), 1000.0f);
        set (*strip, ParamIDs::eqGain ("hmf"), 12.0f);
        // A tone outside the sweep's path, so the bell doesn't sweep across it.
        checkSwitchIsSmooth (*strip, 500.0f, [&] { set (*strip, ParamIDs::eqFreq ("hmf"), 4000.0f); });
    }
    SECTION ("the HPF on")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::hpfFreq, 1000.0f);
        checkSwitchIsSmooth (*strip, 300.0f, [&] { set (*strip, ParamIDs::hpfOn, 1.0f); });
    }
    SECTION ("the filters moved from IN to SC")
    {
        auto strip = makeStrip();
        set (*strip, ParamIDs::hpfOn, 1.0f);
        set (*strip, ParamIDs::hpfFreq, 1000.0f);
        checkSwitchIsSmooth (*strip, 300.0f, [&] { set (*strip, ParamIDs::filterPlacement, 2.0f); });
    }
}

TEST_CASE ("Frequencies read as Hz and kHz", "[eq]")
{
    CHECK (Parameters::frequencyText (120.4f) == "120 Hz");
    CHECK (Parameters::frequencyText (2500.0f) == "2.50 kHz");
    CHECK (Parameters::frequencyText (12000.0f) == "12.0 kHz");
}

