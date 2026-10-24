#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "LevelMeterSource.h"
#include "Parameters.h"

// The channel strip. True stereo (stereo in, stereo out), or mono in, mono
// out on a mono track. For now it has INPUT, the OUTPUT column (Ø, MUTE,
// OUTPUT TRIM and the fader) and the meters; the modules are built one at a
// time (see CLAUDE.md).
class ParagonPR7CAudioProcessor final : public juce::AudioProcessor
{
public:
    ParagonPR7CAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    // Peak levels for the meters, per channel (0 left, 1 right; on a mono
    // track both show the one channel).
    LevelMeterSource& getInputMeter (int channel) noexcept { return inputMeters[(size_t) juce::jlimit (0, 1, channel)]; }
    LevelMeterSource& getOutputMeter (int channel) noexcept { return outputMeters[(size_t) juce::jlimit (0, 1, channel)]; }

    // Gain reduction for the GR meters, per dynamics module and channel.
    enum class Dynamics { gate, comp, limit };
    GainReductionSource& getGainReduction (Dynamics dynamics, int channel) noexcept
    {
        return gainReduction[(size_t) dynamics][(size_t) juce::jlimit (0, 1, channel)];
    }

private:
    void pushMeters (std::array<LevelMeterSource, 2>&, const juce::AudioBuffer<float>&, int numSamples) noexcept;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* outputLevel = nullptr;
    std::atomic<float>* inputGainDb = nullptr;
    std::atomic<float>* outputTrim = nullptr;
    std::atomic<float>* phaseInvert = nullptr;
    std::atomic<float>* mute = nullptr;

    // Every gain change glides over 20 ms, so nothing clicks. The output's
    // polarity, trim, fader and mute are one gain: flipping the polarity
    // glides through zero to -1, and muting glides down to zero.
    juce::SmoothedValue<float> inputGain { 1.0f }, outputGain { 1.0f };
    float targetOutputGain() const noexcept;

    std::array<LevelMeterSource, 2> inputMeters, outputMeters;
    std::array<std::array<GainReductionSource, 2>, 3> gainReduction;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParagonPR7CAudioProcessor)
};
