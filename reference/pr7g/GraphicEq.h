#pragma once

#include "EffectUtils.h"

namespace effects
{
    // A 9-band graphic EQ, one band per octave from 63 Hz to 16 kHz, each a
    // peaking filter that can cut or boost by up to 12 dB, plus an output
    // level. It sits after the cabinet, where hardware modellers put theirs,
    // to shape the amp and cab's tone before modulation, delay and reverb.
    class GraphicEq
    {
    public:
        static constexpr int numBands = 9;
        static constexpr std::array<float, numBands> frequencies { 63.0f, 125.0f, 250.0f, 500.0f, 1000.0f,
                                                                   2000.0f, 4000.0f, 8000.0f, 16000.0f };
        static constexpr float maxGainDb = 12.0f;

        // How wide each band is. 1.2 makes neighbouring bands blend into a
        // smooth curve when several are moved together, as on a hardware
        // graphic EQ, rather than leaving dips between them.
        static constexpr float bandQ = 1.2f;

        struct Settings
        {
            bool on = false;
            std::array<float, numBands> gainsDb {};
            float levelDb = 0.0f;
        };

        void prepare (double sampleRate, int maxBlockSize);
        void process (juce::dsp::AudioBlock<float> monoBlock, const Settings&) noexcept;

        // The EQ's response at `frequency` in dB, for drawing its curve.
        static float responseDb (const std::array<float, numBands>& gainsDb, float frequency, double sampleRate = 48000.0);

        // A band's frequency, kept safely below half the sample rate.
        static float bandFrequency (int band, double sampleRate) noexcept;

    private:
        double sampleRate = 48000.0;
        std::array<juce::dsp::IIR::Filter<float>, numBands> filters;
        std::array<juce::SmoothedValue<float>, numBands> gains;   // in dB
        std::array<float, numBands> appliedDb {};
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> level;
        BypassFade bypass;
    };
}
