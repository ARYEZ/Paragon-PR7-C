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
            REQUIRE_THAT (output.getSample (channel, i), WithinAbs (delayedInput (input, channel, i), 1.0e-6));
}

TEST_CASE ("Defaults read back exactly, so modules at their defaults switch themselves off", "[processor]")
{
    // A stepped knob's 0 must be exactly 0, not a rounding hair off it (as
    // a fused multiply-add on Apple silicon once made it): an EQ band at
    // 0.0000002 dB still runs.
    ParagonPR7CAudioProcessor processor;
    for (auto* parameter : processor.getParameters().processor.getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter);
        REQUIRE (ranged != nullptr);
        INFO (ranged->getParameterID().toStdString());
        const auto raw = processor.getParameters().getRawParameterValue (ranged->getParameterID())->load();
        if (std::abs (raw) < 0.01f)
            CHECK (juce::exactlyEqual (raw, 0.0f));
    }
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

TEST_CASE ("INPUT sets the level into the strip, and the IN meters show it", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);
    setParameter (processor, ParamIDs::inputGain, -6.0f);

    const auto input = stereoTone (9600);
    auto output = input;
    process (processor, output);

    const auto expected = juce::Decibels::decibelsToGain (-6.0f);
    for (int channel = 0; channel < 2; ++channel)
        CHECK_THAT (output.getRMSLevel (channel, 4800, 4800) / input.getRMSLevel (channel, 4800, 4800), WithinAbs (expected, 1.0e-3));

    // Once INPUT has glided to -6 dB, the IN meter reads the tone 6 dB down.
    processor.getInputMeter (0).takePeak();
    auto more = stereoTone (2400);
    process (processor, more);
    CHECK_THAT (processor.getInputMeter (0).takePeak(), WithinAbs (0.5 * expected, 0.01));
}

TEST_CASE ("INPUT's 0 dB sits at the top of the knob", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    auto* input = processor.getParameters().getParameter (ParamIDs::inputGain);
    CHECK_THAT (input->convertTo0to1 (0.0f), WithinAbs (0.5, 1.0e-6));
    CHECK_THAT (input->convertTo0to1 (-24.0f), WithinAbs (0.0, 1.0e-6));
    CHECK_THAT (input->convertTo0to1 (12.0f), WithinAbs (1.0, 1.0e-6));
    CHECK_THAT (input->convertFrom0to1 (0.25f), WithinAbs (-12.0, 1.0e-4));
}

TEST_CASE ("OUTPUT TRIM and the fader add up", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);
    setParameter (processor, ParamIDs::outputTrim, 3.0f);
    setParameter (processor, ParamIDs::outputLevel, -9.0f);

    const auto input = stereoTone (9600);
    auto output = input;
    process (processor, output);

    const auto expected = juce::Decibels::decibelsToGain (-6.0f);
    for (int channel = 0; channel < 2; ++channel)
        CHECK_THAT (output.getRMSLevel (channel, 4800, 4800) / input.getRMSLevel (channel, 4800, 4800), WithinAbs (expected, 1.0e-3));
}

TEST_CASE ("The fader's travel follows its printed scale", "[processor]")
{
    // The marks are evenly spaced, top (+10) to bottom (-60).
    const auto numMarks = (int) std::size (ParamRanges::faderMarksDb);
    for (int i = 0; i < numMarks; ++i)
        CHECK_THAT (Parameters::faderProportion (ParamRanges::faderMarksDb[i]), WithinAbs (1.0 - (double) i / (numMarks - 1), 1.0e-6));

    for (float db = -60.0f; db <= 10.0f; db += 0.7f)
        CHECK_THAT (Parameters::faderDb (Parameters::faderProportion (db)), WithinAbs (db, 1.0e-3));

    // The host sees the same travel (automation lanes match the fader).
    ParagonPR7CAudioProcessor processor;
    auto* fader = processor.getParameters().getParameter (ParamIDs::outputLevel);
    CHECK_THAT (fader->convertTo0to1 (0.0f), WithinAbs (Parameters::faderProportion (0.0f), 1.0e-6));
    CHECK_THAT (fader->convertTo0to1 (-20.0f), WithinAbs (Parameters::faderProportion (-20.0f), 1.0e-6));
}

TEST_CASE ("Phase invert flips the polarity of both channels", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);
    setParameter (processor, ParamIDs::phaseInvert, 1.0f);

    const auto input = stereoTone (9600);
    auto output = input;
    process (processor, output);

    // After the 20 ms glide, every sample is the input's, upside down.
    for (int channel = 0; channel < 2; ++channel)
        for (int i = 4800; i < 9600; ++i)
            REQUIRE_THAT (output.getSample (channel, i), WithinAbs (-delayedInput (input, channel, i), 1.0e-6));
}

TEST_CASE ("Flipping phase invert or MUTE mid-signal glides instead of clicking", "[processor]")
{
    for (const auto* id : { ParamIDs::phaseInvert, ParamIDs::mute })
    {
        DYNAMIC_SECTION (id)
        {
            ParagonPR7CAudioProcessor processor;
            processor.prepareToPlay (sampleRate, blockSize);

            const auto input = stereoTone (9600);
            auto output = input;
            juce::MidiBuffer midi;
            for (int start = 0; start < output.getNumSamples(); start += blockSize)
            {
                if (start == 2560)   // switched part-way through the tone
                    setParameter (processor, id, 1.0f);
                juce::AudioBuffer<float> block (output.getArrayOfWritePointers(), 2, start, juce::jmin (blockSize, output.getNumSamples() - start));
                processor.processBlock (block, midi);
            }

            // The biggest jump between neighbouring samples stays close to
            // the tone's own (an instant flip would jump by up to twice the
            // level). The 440 Hz tone at 0.5 moves at most 0.029 a sample.
            float biggestJump = 0.0f;
            for (int i = 1; i < output.getNumSamples(); ++i)
                biggestJump = juce::jmax (biggestJump, std::abs (output.getSample (0, i) - output.getSample (0, i - 1)));
            CHECK (biggestJump < 0.032f);
        }
    }
}

TEST_CASE ("MUTE silences the output", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);
    setParameter (processor, ParamIDs::mute, 1.0f);

    auto output = stereoTone (9600);
    process (processor, output);
    for (int channel = 0; channel < 2; ++channel)
        CHECK (output.getMagnitude (channel, 4800, 4800) == 0.0f);

    // Once it has faded out, the OUT meters see nothing.
    processor.getOutputMeter (0).takePeak();
    auto more = stereoTone (2400);
    process (processor, more);
    CHECK (processor.getOutputMeter (0).takePeak() == 0.0f);
}

TEST_CASE ("The fader at the very bottom is silence", "[processor]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);
    setParameter (processor, ParamIDs::outputLevel, ParamRanges::outputLevelMin);

    auto output = stereoTone (9600);
    process (processor, output);
    for (int channel = 0; channel < 2; ++channel)
        CHECK (output.getMagnitude (channel, 4800, 4800) == 0.0f);
    CHECK (processor.getParameters().getParameter (ParamIDs::outputLevel)->getCurrentValueAsText() == "-inf dB");
}

namespace
{
    // How many samples later a quiet sine comes out (its phase delay). A
    // phase can't tell delays a whole cycle apart, so the one nearest
    // `expected`.
    double measuredDelay (ParagonPR7CAudioProcessor& processor, float frequency, double expected)
    {
        juce::AudioBuffer<float> buffer (2, 48000);
        const auto omega = juce::MathConstants<double>::twoPi * frequency / sampleRate;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            for (int channel = 0; channel < 2; ++channel)
                buffer.setSample (channel, i, (float) (0.01 * std::sin (omega * i)));
        process (processor, buffer);
        double sine = 0.0, cosine = 0.0;
        for (int i = 24000; i < buffer.getNumSamples(); ++i)
        {
            sine += buffer.getSample (0, i) * std::sin (omega * i);
            cosine += buffer.getSample (0, i) * std::cos (omega * i);
        }
        const auto cycle = sampleRate / frequency;
        auto delay = -std::atan2 (cosine, sine) / omega;
        delay += cycle * std::round ((expected - delay) / cycle);
        return delay;
    }
}

TEST_CASE ("The strip's delay is reported to the host, and the same whatever is on", "[processor]")
{
    // SATURATION and THD run oversampled, and their filters delay the sound
    // a little; while they're off, the sound is delayed as much instead, so
    // two tracks with different settings still line up.
    const auto check = [] (bool saturation, bool thd)
    {
        ParagonPR7CAudioProcessor processor;
        processor.prepareToPlay (sampleRate, blockSize);
        CHECK (processor.getLatencySamples() == ParagonPR7CAudioProcessor::latencySamples);
        setParameter (processor, ParamIDs::satOn, saturation ? 1.0f : 0.0f);
        setParameter (processor, ParamIDs::toneThd, thd ? 100.0f : 0.0f);
        for (const auto frequency : { 1000.0f, 5000.0f })
        {
            INFO ("saturation " << saturation << ", THD " << thd << ", " << frequency << " Hz");
            // Off, exactly; on, within a fraction of a sample (the filters'
            // delay isn't a whole number of samples).
            CHECK_THAT (measuredDelay (processor, frequency, ParagonPR7CAudioProcessor::latencySamples), WithinAbs (ParagonPR7CAudioProcessor::latencySamples, saturation || thd ? 0.8 : 0.01));
        }
    };
    check (false, false);
    check (true, false);
    check (false, true);
    check (true, true);
}

TEST_CASE ("The delays the strip makes up for match its oversampling filters", "[processor]")
{
    // If the oversampling changes, these must too.
    using Oversampling = juce::dsp::Oversampling<float>;
    Oversampling saturation (2, effects::Saturation::oversamplingStages, Oversampling::filterHalfBandPolyphaseIIR, true, false);
    Oversampling thd (2, 1, Oversampling::filterHalfBandPolyphaseIIR, true, false);
    CHECK (juce::roundToInt (saturation.getLatencyInSamples()) == effects::Saturation::latencySamples);
    CHECK (juce::roundToInt (thd.getLatencyInSamples()) == effects::ConsoleColour::latencySamples);
}
