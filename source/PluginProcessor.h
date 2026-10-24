#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "ChannelRegistry.h"
#include "LevelMeterSource.h"
#include "Parameters.h"
#include "PresetManager.h"
#include "dsp/ChannelEq.h"
#include "dsp/Compressor.h"
#include "dsp/ConsoleChannel.h"
#include "dsp/ConsoleColour.h"
#include "dsp/Gate.h"
#include "dsp/Limiter.h"
#include "dsp/Saturation.h"
#include "dsp/StereoTools.h"

// The channel strip. True stereo (stereo in, stereo out), or mono in, mono
// out on a mono track. So far: INPUT, the HPF / LPF, the gate / expander
// and SC FILTER, the compressor, the five-band EQ, the limiter, the
// saturation, the console colour (THD, V GAIN and the console channels),
// MONO MAKER, STEREO WIDTH and the STEREO MODE, the OUTPUT column (Ø, MUTE, OUTPUT TRIM and the fader) and
// the meters. The other modules are built one at a time (see CLAUDE.md).
class ParagonPR7CAudioProcessor final : public juce::AudioProcessor
{
public:
    ParagonPR7CAudioProcessor();
    ~ParagonPR7CAudioProcessor() override;

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
    // Factory and user presets (the header). Message thread only.
    PresetManager& getPresets() noexcept { return presets; }

    // Peak levels for the meters, per channel (0 left, 1 right; on a mono
    // track both show the one channel).
    LevelMeterSource& getInputMeter (int channel) noexcept { return inputMeters[(size_t) juce::jlimit (0, 1, channel)]; }
    LevelMeterSource& getOutputMeter (int channel) noexcept { return outputMeters[(size_t) juce::jlimit (0, 1, channel)]; }

    // Gain reduction for the GR meters, per dynamics module and channel.
    enum class Dynamics { gate, comp, limit };
    // For COMP's own gain-change meter: its reduction (the louder channel),
    // and its make-up gain (dB, 0 while it's off). Separate from the meter
    // window's COMP meters, as each reading empties a source.
    GainReductionSource& getCompGainChange() noexcept { return compGainChange; }
    const std::atomic<float>& getCompMakeUp() const noexcept { return compMakeUpShown; }
    GainReductionSource& getGainReduction (Dynamics dynamics, int channel) noexcept
    {
        return gainReduction[(size_t) dynamics][(size_t) juce::jlimit (0, 1, channel)];
    }

    // The console channels: the left side's (1 to 72), and whether the
    // right side uses the next one (ANALOG) or the same one (DIGITAL).
    // setConsoleChannel is for the message thread (RANDOM CH.).
    int getConsoleChannel() const noexcept;
    bool isAnalog() const noexcept;
    void setConsoleChannel (int);

private:
    void pushMeters (std::array<LevelMeterSource, 2>&, const juce::AudioBuffer<float>&, int numSamples) noexcept;
    // The modules, in their fixed order, on a stretch no longer than the
    // block size prepareToPlay was given.
    void processModules (juce::AudioBuffer<float>&, int numSamples) noexcept;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* outputLevel = nullptr;
    std::atomic<float>* inputGainDb = nullptr;
    std::atomic<float>* outputTrim = nullptr;
    std::atomic<float>* phaseInvert = nullptr;
    std::atomic<float>* mute = nullptr;

    // The HPF / LPF, in each place they can sit in the chain (IN and POST;
    // in SC they leave the audio alone), and the EQ bands, top to bottom.
    struct FilterParameters { std::atomic<float>* hpfOn, * hpfFrequency, * lpfOn, * lpfFrequency, * placement; };
    struct BandParameters { std::atomic<float>* on, * frequency, * gain, * q, * shelf; };   // q or shelf is null
    // Each side has its own, as each side can be a different console
    // channel, with slightly different frequencies and gains.
    FilterParameters filterParameters {};
    std::array<BandParameters, std::size (ParamRanges::eqBands)> bandParameters {};
    using EqBands = std::array<effects::EqBand, std::size (ParamRanges::eqBands)>;
    static EqBands makeEqBands();
    std::array<effects::CutFilters, 2> filtersIn, filtersPost;
    std::array<EqBands, 2> eqBands { makeEqBands(), makeEqBands() };
    effects::CutFilters::Settings filterSettings (int placement, const effects::ConsoleChannel& = {}) const noexcept;
    // The HPF / LPF or the EQ, run on each side with its channel's differences.
    void processFilters (std::array<effects::CutFilters, 2>&, juce::AudioBuffer<float>&, int numSamples, int placement) noexcept;
    void processEq (juce::AudioBuffer<float>&, int numSamples) noexcept;

    // FILTERS / TONE: the console colour, and each side's console channel
    // (all the design's own values while the module is off).
    struct ToneParameters { std::atomic<float>* on, * thd, * vGain, * channel; };
    ToneParameters toneParameters {};
    effects::ConsoleColour colour;
    std::array<effects::ConsoleChannel, 2> sides;
    std::array<int, 2> sideNumbers { 1, 1 };   // the channels `sides` holds (0: none yet)
    void updateSides() noexcept;

    // MONO MAKER / STEREO WIDTH, and the STEREO MODE.
    struct StereoParameters { std::atomic<float>* monoOn, * monoFrequency, * widthOn, * width, * mode; };
    StereoParameters stereoParameters {};
    effects::StereoTools stereoTools;

    PresetManager presets { parameters };

    // The gate and compressor, and their shared sidechain: what their
    // detectors hear (the audio at the gate's input, through the HPF / LPF
    // when they're set to SC, and the SC FILTER). LISTEN plays it.
    struct GateParameters { std::atomic<float>* on, * threshold, * minus30, * range, * expander, * ratio, * attack, * hold, * release, * linear; };
    struct ScParameters { std::atomic<float>* on, * hpf, * lpf, * listen, * highQ; };
    GateParameters gateParameters {};
    ScParameters scParameters {};
    effects::Gate gate;
    effects::CutFilters scFilter;
    juce::SmoothedValue<float> listen { 0.0f };
    struct CompParameters { std::atomic<float>* on, * threshold, * ratio, * attack, * release, * autoRelease, * makeUp, * character, * link; };
    CompParameters compParameters {};
    effects::Compressor compressor;
    GainReductionSource compGainChange;

    struct LimitParameters { std::atomic<float>* on, * threshold, * makeUp, * mode, * mix; };
    LimitParameters limitParameters {};
    effects::Limiter limiter;

    struct SatParameters { std::atomic<float>* on, * drive, * harmonics, * vcaClip, * mix; };
    SatParameters satParameters {};
    effects::Saturation saturation;
    std::atomic<float> compMakeUpShown { 0.0f };
    effects::CutFilters filtersSidechain;
    juce::AudioBuffer<float> sidechain;
    int maxBlockSize = 0;

    // Every gain change glides over 20 ms, so nothing clicks. The output's
    // polarity, trim, fader and mute are one gain: flipping the polarity
    // glides through zero to -1, and muting glides down to zero.
    juce::SmoothedValue<float> inputGain { 1.0f }, outputGain { 1.0f };
    float targetOutputGain() const noexcept;

    std::array<LevelMeterSource, 2> inputMeters, outputMeters;
    std::array<std::array<GainReductionSource, 2>, 3> gainReduction;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParagonPR7CAudioProcessor)
};
