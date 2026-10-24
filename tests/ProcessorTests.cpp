#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Parameters.h"
#include "PluginProcessor.h"

using Catch::Matchers::WithinAbs;

namespace
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;

    // A different sine on each channel, so a swapped or mixed channel shows.
    juce::AudioBuffer<float> stereoTone (int numSamples)
    {
        juce::AudioBuffer<float> buffer (2, numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            buffer.setSample (0, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / (float) sampleRate));
            buffer.setSample (1, i, 0.25f * std::sin (juce::MathConstants<float>::twoPi * 1000.0f * (float) i / (float) sampleRate));
        }
        return buffer;
    }

    void setParameter (ParagonPR7CAudioProcessor& processor, const char* id, float value)
    {
        auto* parameter = processor.getParameters().getParameter (id);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }

    void process (ParagonPR7CAudioProcessor& processor, juce::AudioBuffer<float>& buffer)
    {
        juce::MidiBuffer midi;
        for (int start = 0; start < buffer.getNumSamples(); start += blockSize)
        {
            juce::AudioBuffer<float> block (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start,
                                            juce::jmin (blockSize, buffer.getNumSamples() - start));
            processor.processBlock (block, midi);
        }
    }
}

TEST_CASE ("At its defaults the strip passes stereo audio through untouched", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);

    const auto input = stereoTone (4800);
    auto output = input;
    process (processor, output);

    for (int channel = 0; channel < 2; ++channel)
        for (int i = 0; i < input.getNumSamples(); ++i)
            REQUIRE_THAT (output.getSample (channel, i), WithinAbs (input.getSample (channel, i), 1.0e-6));
}

TEST_CASE ("OUTPUT LEVEL sets the level, on both channels", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);
    setParameter (processor, ParamIDs::outputLevel, -6.0f);

    const auto input = stereoTone (9600);
    auto output = input;
    process (processor, output);

    // After the 20 ms glide: -6 dB is half the amplitude.
    const auto expected = juce::Decibels::decibelsToGain (-6.0f);
    for (int channel = 0; channel < 2; ++channel)
        CHECK_THAT (output.getRMSLevel (channel, 4800, 4800) / input.getRMSLevel (channel, 4800, 4800), WithinAbs (expected, 1.0e-3));
}

TEST_CASE ("Mono and stereo tracks are supported, mismatched layouts aren't", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (juce::AudioChannelSet::stereo());
    layout.outputBuses.add (juce::AudioChannelSet::stereo());
    CHECK (processor.checkBusesLayoutSupported (layout));

    layout.inputBuses.getReference (0) = juce::AudioChannelSet::mono();
    layout.outputBuses.getReference (0) = juce::AudioChannelSet::mono();
    CHECK (processor.checkBusesLayoutSupported (layout));

    layout.outputBuses.getReference (0) = juce::AudioChannelSet::stereo();
    CHECK (! processor.checkBusesLayoutSupported (layout));
}

TEST_CASE ("Settings are saved with the project and restored", "[processor]")
{
    juce::MemoryBlock saved;
    {
        ParagonPR7CAudioProcessor processor;
        setParameter (processor, ParamIDs::outputLevel, 4.5f);
        processor.getStateInformation (saved);
    }
    ParagonPR7CAudioProcessor restored;
    restored.setStateInformation (saved.getData(), (int) saved.getSize());
    CHECK_THAT (restored.getParameters().getRawParameterValue (ParamIDs::outputLevel)->load(), WithinAbs (4.5, 0.05));
}

TEST_CASE ("The meters see the input and the output", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);
    setParameter (processor, ParamIDs::outputLevel, -12.0f);

    auto buffer = stereoTone (9600);
    process (processor, buffer);

    CHECK_THAT (processor.getInputMeter (0).takePeak(), WithinAbs (0.5, 0.01));
    CHECK_THAT (processor.getInputMeter (1).takePeak(), WithinAbs (0.25, 0.01));
    CHECK (processor.getOutputMeter (0).takePeak() < 0.5f);
}
